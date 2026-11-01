#include "nvme.h"
#include "pci.h"
#include "kalloc.h"
#include "vmm.h"
#include "puts.h"

static NVME_DEVICE g_nvme_device;
static NVME_IO_QUEUE g_nvme_io;

uint8_t PciScanNvme(PPCI_DEVICE device)
{
  //enumerate
  for(uint16_t bus = 0; bus < 256; bus++)
  {
    for(uint8_t dev = 0; dev < 32; dev++)
    {
      for(uint8_t func = 0; func < 8; func++)
      {
        HEADER_FIELD ID = { 0 };
	ID.Value = PciConfigRead_DWORD(bus, dev, func, 0x0);
	if(ID.VendorID == 0xFFFF)
	{
	  if(func == 0) break;
	  continue;
	}

        HEADER_FIELD DeviceType = { 0 };
	DeviceType.Value = PciConfigRead_DWORD(bus, dev, func, 0x08);
	if(DeviceType.ClassCode == 0x1 && DeviceType.SubClass == 0x8 && DeviceType.ProgIF == 0x2)
	{
	  device->Bus = bus;
	  device->Device = dev;
	  device->Function = func;
	  device->VendorID = ID.VendorID;
	  device->DeviceID = ID.DeviceID;
	  device->ClassCode = DeviceType.ClassCode;
	  device->SubClass = DeviceType.SubClass;
	  device->ProgIF = DeviceType.ProgIF;
	  device->BAR0 = PciGetBAR0(bus, dev, func);

	  PciEnableFeatures(bus, dev, func);

	  puts("Find NVMe\n");
	  puts("bus : ");puts_hex(bus);
	  puts(", device : ");puts_hex(dev);
	  puts(", function : ");puts_hex(func);
	  puts(", BAR PA : ");puts_hex(device->BAR0);
	  puts("\n");
	  return 0;
	}
      }
    }
  }
  return 1;
}

uint8_t NvmeInit(uint64_t bar0_pa)
{
  uintptr_t mmio = (uintptr_t)map_mmio(bar0_pa, 0x10000); // 64KB
  volatile PCONTROLLER_REGISTER ctrl_reg = (volatile PCONTROLLER_REGISTER)mmio;

  g_nvme_device.BAR0_VA = mmio;

  if(ctrl_reg->CSTS.RDY)
  {
    ctrl_reg->CC.EN = 0;
  }

  while(ctrl_reg->CSTS.RDY)
  {
    // need to identify timeout
    __asm__ volatile("pause");
  }

  g_nvme_device.DSTRD = ctrl_reg->CAP.DSTRD;
  uint32_t mqes = ctrl_reg->CAP.MQES + 1;

  void *asq_page = kalloc(); //They can support more page
  void *acq_page = kalloc();

  memset(asq_page, 0, PGSIZE);
  memset(acq_page, 0, PGSIZE);

  g_nvme_device.ASQ_VA = (PCOMMON_CMD)asq_page;
  g_nvme_device.ASQ_PA = (uint64_t)v2p(asq_page);
  g_nvme_device.SQ_TAIL = 0;

  g_nvme_device.ACQ_VA = (PCOMMAND_CQE)acq_page;
  g_nvme_device.ACQ_PA = (uint64_t)v2p(acq_page);
  g_nvme_device.CQ_HEAD = 0;
  g_nvme_device.CQ_PHASE = 1;

  ctrl_reg->AQA.ASQS = ADMIN_QUEUE_SIZE - 1;
  ctrl_reg->AQA.ACQS = ADMIN_QUEUE_SIZE - 1;
  ctrl_reg->ASQ.Value = g_nvme_device.ASQ_PA; // PFN
  ctrl_reg->ACQ.Value = g_nvme_device.ACQ_PA; // PFN

  g_nvme_device.ASQ_DB = NvmeGetSQ(mmio, 0, g_nvme_device.DSTRD);
  g_nvme_device.ACQ_DB = NvmeGetCQ(mmio, 0, g_nvme_device.DSTRD);

  ctrl_reg->CC.EN = 1;
  ctrl_reg->CC.CSS = 0;
  ctrl_reg->CC.MPS = 0;
  ctrl_reg->CC.AMS = 0;
  ctrl_reg->CC.IOSQES = 6;
  ctrl_reg->CC.IOCQES = 4;

  while(!ctrl_reg->CSTS.RDY)
  {
    if(ctrl_reg->CSTS.CFS)
    {
      puts("Error : NVMe Fatal Status Detected!!!\n");
      return 1;
    }
    // identify timeout
    __asm__ volatile("pause");
  }

  puts("INFO : NVMe is ready\n");
  return 0;
}

uint16_t NvmeAdminCommand(PCOMMON_CMD cmd, PCOMMAND_CQE cqe_o)
{
  uint16_t tail = g_nvme_device.SQ_TAIL;

  cmd->CDW0.CID = tail;
  memcpy((void*)&g_nvme_device.ASQ_VA[tail], cmd, sizeof(COMMON_CMD));

  __asm__ volatile("sfence" ::: "memory");

  tail = (tail + 1) % ADMIN_QUEUE_SIZE;
  g_nvme_device.SQ_TAIL = tail;
  *g_nvme_device.ASQ_DB = tail; // mmio

  volatile PCOMMAND_CQE cqe = &g_nvme_device.ACQ_VA[g_nvme_device.CQ_HEAD];

  while(cqe->DW3.P != g_nvme_device.CQ_PHASE)
  {
    __asm__ volatile("pause");
  }

  __asm__ volatile("lfence" ::: "memory");

  if(cqe_o)
  {
    memcpy(cqe_o, (void*)cqe, sizeof(COMMAND_CQE));
  }

  uint16_t next_head = (g_nvme_device.CQ_HEAD + 1) % ADMIN_QUEUE_SIZE;
  if(next_head == 0)
  {
    g_nvme_device.CQ_PHASE ^= 1;
  }
  g_nvme_device.CQ_HEAD = next_head;
  
  __asm__ volatile("sfence" ::: "memory");
  *g_nvme_device.ACQ_DB = next_head; //mmio

  return cqe->DW3.STATUS;
}

void NvmeIdentifyTest(void)
{
  void *buffer_va = kalloc();
  memset(buffer_va, 0, PGSIZE);
  uint64_t buffer_pa = (uint64_t)v2p(buffer_va);

  COMMON_CMD cmd = { 0 };
  cmd.CDW0.OPC = 0x6;   // Identify
  cmd.NSID = 0;         // Identify Controller must be 0
  cmd.PRP1 = buffer_pa;
  cmd.PRP2 = 0;
  cmd.CDW10 = 1;        // CNS = 1; Identify Controller

  COMMAND_CQE cqe = { 0 };
  if(NvmeAdminCommand(&cmd, &cqe))
  {
    puts("ERROR : NVMe Identify Controller FAILED\n");
    return;
  }

  char model[41];
  char serial[21];

  // model : 24-63 bit
  memcpy(model, (char*)buffer_va + 24, 40);
  model[40] = '\0';

  // serial : 4-23 bit
  memcpy(serial, (char*)buffer_va + 4, 20);
  serial[20] = '\0';

  puts("NVMe Controller Identified Successfully\n");
  puts("Model : "); puts(model); puts("\n");
  puts("Serial : "); puts(serial); puts("\n");
  return;
}

uint8_t NvmeInitIoQueues(void)
{
  g_nvme_io.IOCQ_VA = (PCOMMAND_CQE)kalloc();
  g_nvme_io.IOSQ_VA = (PCOMMON_CMD)kalloc();
  memset(g_nvme_io.IOCQ_VA, 0, PGSIZE);
  memset(g_nvme_io.IOSQ_VA, 0, PGSIZE);

  g_nvme_io.IOCQ_PA = (uint64_t)v2p(g_nvme_io.IOCQ_VA);
  g_nvme_io.IOSQ_PA = (uint64_t)v2p(g_nvme_io.IOSQ_VA);
  g_nvme_io.SQ_TAIL = 0;
  g_nvme_io.CQ_HEAD = 0;
  g_nvme_io.CQ_PHASE = 1;

  g_nvme_io.SQ_DB = NvmeGetSQ(g_nvme_device.BAR0_VA, IO_QID, g_nvme_device.DSTRD);
  g_nvme_io.CQ_DB = NvmeGetCQ(g_nvme_device.BAR0_VA, IO_QID, g_nvme_device.DSTRD);

  COMMON_CMD cmd;
  memset(&cmd, 0, sizeof(COMMON_CMD));
  cmd.CDW0.OPC = 0x05;                              // Create IO CQ
  cmd.PRP1 = g_nvme_io.IOCQ_PA;
  cmd.CDW10 = ((IO_QUEUE_SIZE - 1) << 16) | IO_QID; // 31-16 : SIZE - 1, 15-0 : QID
  cmd.CDW11 = 0x1;

  if(NvmeAdminCommand(&cmd, 0))
  {
    puts("ERROR : Failed to create IO CQ\n");
    return 1;
  }

  memset(&cmd, 0, sizeof(COMMON_CMD));
  cmd.CDW0.OPC = 0x01;                              // Create IO SQ
  cmd.PRP1 = g_nvme_io.IOSQ_PA;
  cmd.CDW10 = ((IO_QUEUE_SIZE - 1) << 16) | IO_QID; // 31-16 : SIZE - 1, 15-0 : QID
  cmd.CDW11 = (IO_QID << 16) | 0x1UL;

  if(NvmeAdminCommand(&cmd, 0))
  {
    puts("ERROR : Failed to create IO SQ\n");
    return 1;
  }

  puts("INFO : IO Queue created successfully\n");
  return 0;
}

uint16_t NvmeIoCommand(PCOMMON_CMD cmd)
{
  uint16_t tail = g_nvme_io.SQ_TAIL;

  cmd->CDW0.CID = tail;
  memcpy((void*)&g_nvme_io.IOSQ_VA[tail], cmd, sizeof(COMMON_CMD));

  __asm__ volatile("sfence" ::: "memory");

  tail = (tail + 1) % IO_QUEUE_SIZE;
  g_nvme_io.SQ_TAIL = tail;
  *g_nvme_io.SQ_DB = tail; // mmio

  volatile PCOMMAND_CQE cqe = &g_nvme_io.IOCQ_VA[g_nvme_io.CQ_HEAD];

  while(cqe->DW3.P != g_nvme_io.CQ_PHASE)
  {
    __asm__ volatile("pause");
  }

  __asm__ volatile("lfence" ::: "memory");

  uint16_t next_head = (g_nvme_io.CQ_HEAD + 1) % IO_QUEUE_SIZE;
  if(next_head == 0)
  {
    g_nvme_io.CQ_PHASE ^= 1;
  }
  g_nvme_io.CQ_HEAD = next_head;
  
  __asm__ volatile("sfence" ::: "memory");
  *g_nvme_io.CQ_DB = next_head; //mmio

  return cqe->DW3.STATUS;
}

uint16_t NvmeRead(uint64_t lba, uint64_t sectors_count, uint64_t buffer_pa)
{
  if(sectors_count == 0) return 1;
  
  COMMON_CMD cmd;
  memset(&cmd, 0, sizeof(COMMON_CMD));

  cmd.CDW0.OPC = 0x02;                      // NVMe IO Read
  cmd.NSID = 1;                             // NameSpace 1
  cmd.PRP1 = buffer_pa;                     // DMA Buffer PA
  cmd.PRP2 = 0;
  cmd.CDW10 = (uint32_t)(lba & 0xFFFFFFFF); // LBA Low
  cmd.CDW11 = (uint32_t)(lba >> 32);        // LBA High
  cmd.CDW12 = (sectors_count - 1) & 0xFFFF; // 0-based
  
  return NvmeIoCommand(&cmd);
}
uint16_t NvmeWrite(uint64_t lba, uint64_t sectors_count, uint64_t buffer_pa)
{
  if(sectors_count == 0) return 1;
  
  COMMON_CMD cmd;
  memset(&cmd, 0, sizeof(COMMON_CMD));

  cmd.CDW0.OPC = 0x01;                      // NVMe IO Write
  cmd.NSID = 1;                             // NameSpace 1
  cmd.PRP1 = buffer_pa;                     // DMA Buffer PA
  cmd.PRP2 = 0;
  cmd.CDW10 = (uint32_t)(lba & 0xFFFFFFFF); // LBA Low
  cmd.CDW11 = (uint32_t)(lba >> 32);        // LBA High
  cmd.CDW12 = (sectors_count - 1) & 0xFFFF; // 0-based
  
  return NvmeIoCommand(&cmd);
}

void NvmeReadWriteTest(void)
{
  char *buffer_va = (char*)kalloc();
  uint64_t buffer_pa = (uint64_t)v2p(buffer_va);
  memset(buffer_va, 0, PGSIZE);

  char msg[] = "Hello ZAS-MiniOS";
  for(uint32_t i = 0; msg[i] != '\0'; i++)
  {
    buffer_va[i] = msg[i];
  }

  if(NvmeWrite(10, 1, buffer_pa))
  {
    puts("ERROR : Write Failed\n");
    return;
  }

  memset(buffer_va, 0, PGSIZE);

  if(NvmeRead(10, 1, buffer_pa))
  {
    puts("ERROR : Read Failed");
    return;
  }

  puts("Content from disk :\n");
  puts(buffer_va);
  puts("\n");
  return;
}
