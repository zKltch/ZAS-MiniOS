#include "pci.h"
#include "puts.h"

// All the magic number is the offset of the struct
// The offset of the struct is controlled by Register_Offset

uint32_t PciConfigRead_DWORD(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset)
{
  CONFIG_ADDRESS address = { 0 };
  address.Enable_Bit = 1;
  address.Bus_Number = bus;
  address.Device_Number = dev;
  address.Function_Number = func;
  address.Register_Offset = offset & 0xFC; //bit 0-1 must be zero

  outl(PCI_CONFIG_ADDRESS, address.Flag);

  return inl(PCI_CONFIG_DATA);
}

void PciConfigWrite_DWORD(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint32_t val)
{
  CONFIG_ADDRESS address = { 0 };
  address.Enable_Bit = 1;
  address.Bus_Number = bus;
  address.Device_Number = dev;
  address.Function_Number = func;
  address.Register_Offset = offset & 0xFC;

  outl(PCI_CONFIG_ADDRESS, address.Flag);

  outl(PCI_CONFIG_DATA, val);
  return;
}

void PciEnableFeatures(uint8_t bus, uint8_t dev, uint8_t func)
{
  HEADER_FIELD cmd_status = { 0 };
  cmd_status.Value = PciConfigRead_DWORD(bus, dev, func, 0x4);
  cmd_status.Command.Memory_Space = 1;
  cmd_status.Command.Bus_Master = 1;

  PciConfigWrite_DWORD(bus, dev, func, 0x4, cmd_status.Value);

  return;
}

uint64_t PciGetBAR(uint8_t bus, uint8_t dev, uint8_t func)
{
  HEADER_FIELD BAR0_field = { 0 };
  BAR0_field.Value = PciConfigRead_DWORD(bus, dev, func, 0x10);

  if(BAR0_field.HEADER_TYPE_0.BAR0.BAR.Type == 0x2) // 64bit
  {
    HEADER_FIELD BAR1_field = { 0 };
    BAR1_field.Value = PciConfigRead_DWORD(bus, dev, func, 0x14);
    return ((uint64_t)BAR1_field.Value << 32) | (BAR0_field.Value & ~0xFULL);
  }
  return (uint64_t)(BAR0_field.Value & ~0xFULL);
}

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
	  device->BAR0 = PciGetBAR(bus, dev, func);

	  PciEnableFeatures(bus, dev, func);

	  puts("Find NVMe\n");
	  puts("bus : ");puts_hex(bus);
	  puts(", device : ");puts_hex(dev);
	  puts(", function : ");puts_hex(func);
	  puts(", BAR PA : ");puts_hex(device->BAR0);
	  puts("\n");
	  return 1;
	}
      }
    }
  }
  return 0;
}
