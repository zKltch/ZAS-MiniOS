#ifndef NVME_H
#define NVME_H

#include <stdint.h>
#include "pci.h"

#define ADMIN_QUEUE_SIZE 64UL
#define IO_QUEUE_SIZE 64UL
#define IO_QID 1UL

#pragma pack(push, 1)
typedef union _CONTROLLER_CAPABILITIES
{
  struct
  {
    uint64_t MQES : 16;
    uint64_t CQR : 1;
    uint64_t AMS : 2;
    uint64_t Reserved1 : 5;
    uint64_t TO : 8;
    uint64_t DSTRD : 4;
    uint64_t NSSRS : 1;
    uint64_t CSS : 8;
    uint64_t BPS : 1;
    uint64_t CPS : 2;
    uint64_t MPSMIN : 4;
    uint64_t MPSMAX : 4;
    uint64_t PMRS : 1;
    uint64_t CMBS : 1;
    uint64_t NSSS : 1;
    uint64_t CRMS : 2;
    uint64_t NSSES : 1;
    uint64_t Reserved2 : 2;
  };
  uint64_t Flag;
}CTRL_CAP, *PCTRL_CAP;

typedef union _CONTROLLER_CONFIGURATION
{
  struct
  {
    uint32_t EN : 1;
    uint32_t Reserved1 : 3;
    uint32_t CSS : 3;
    uint32_t MPS : 4;
    uint32_t AMS : 3;
    uint32_t SHN : 2;
    uint32_t IOSQES : 4;
    uint32_t IOCQES : 4;
    uint32_t CRIME : 1;
    uint32_t Reserved2 : 7;
  };
  uint32_t Flag;
}CTRL_CONFIG, *PCTRL_CONFIG;

typedef union _CONTROLLER_STATUS
{
  struct
  {
    uint32_t RDY : 1;
    uint32_t CFS : 1;
    uint32_t SHST : 2;
    uint32_t NSSRO : 1;
    uint32_t PP : 1;
    uint32_t ST : 1;
    uint32_t Reserved : 25;
  };
  uint32_t Flag;
}CTRL_STATUS, *PCTRL_STATUS;

typedef union _ADMIN_QUEUE_ATTRIBUTES
{
  struct
  {
    uint32_t ASQS : 12;
    uint32_t Reserved1 : 4;
    uint32_t ACQS : 12;
    uint32_t Reserved2 : 4;
  };
  uint32_t Value;
}AQAst, *PAQAst;

typedef union _ADMIN_SUBMISSION_QUEUE_BASE_ADDRESS
{
  struct
  {
    uint64_t Reserved : 12;
    uint64_t ASQB : 52;
  };
  uint64_t Value;
}ASQst, *PASQst;

typedef union _ADMIN_COMPLETION_QUEUE_BASE_ADDRESS
{
  struct
  {
    uint64_t Reserved : 12;
    uint64_t ACQB : 52;
  };
  uint64_t Value;
}ACQst, *PACQst;

// NVMe p79
typedef struct _CONTROLLER_REGISTER
{
  CTRL_CAP CAP;
  uint32_t VS;
  uint32_t INTMS;
  uint32_t INTMC;
  CTRL_CONFIG CC;
  uint32_t Reserved0;
  CTRL_STATUS CSTS;
  uint32_t NSSR;
  AQAst AQA;
  ASQst ASQ;
  ACQst ACQ;
  uint32_t CMBLOC;
  uint32_t CMBSZ;
  uint32_t BPINFO;
  uint32_t BPRSEL;
  uint64_t BPMBL;
  uint64_t CMBMSC;
  uint32_t CMBSTS;
  uint32_t CMBEBS;
  uint32_t CMBSWTP;
  uint32_t NSSD;
  uint32_t CRTO;
  uint8_t reserved2[0xE00 - 0x6C];
  uint32_t PMRCAP;
  uint32_t PMRCTL;
  uint32_t PMRSTS;
  uint32_t PMREBS;
  uint32_t PMRSWTP;
  uint32_t PMRMSCL;
  uint32_t PMRMSCU;
  uint8_t reserved3[0x1000 - 0xE1C];
}CONTROLLER_REGISTER, *PCONTROLLER_REGISTER;

// NVMe p165
typedef union _COMMAND_DWORD_0
{
  struct
  {
    uint32_t OPC : 8; // Opcode
    uint32_t FUSE : 2;
    uint32_t Reserved : 4;
    uint32_t PSDT : 2;
    uint32_t CID : 16;
  };
  uint32_t Value;
}CMD_DWORD_0, *PCMD_DOWRD_0;

typedef union _COMMAND_DWORD_2
{
  struct
  {
    uint16_t SQHD;
    uint16_t SQID;
  };
  uint32_t Value; 
}CMD_DWORD_2, *PCMD_DWORD_2;

typedef union _COMMAND_DWORD_3
{
  struct
  {
    uint32_t CID : 16;
    uint32_t P : 1;
    uint32_t STATUS : 15;
  };
  uint32_t Value;
}CMD_DWORD_3, *PCMD_DWORD_3;

typedef struct _COMMON_COMMAND_FORAMT
{
  CMD_DWORD_0 CDW0;
  uint32_t NSID;
  uint32_t CDW2;
  uint32_t CDW3;
  uint64_t MPTR;
  uint64_t PRP1;
  uint64_t PRP2;
  uint32_t CDW10;
  uint32_t CDW11;
  uint32_t CDW12;
  uint32_t CDW13;
  uint32_t CDW14;
  uint32_t CDW15;
}COMMON_CMD, *PCOMMON_CMD;

typedef struct _COMMAND_CQE
{
  uint32_t Result;   //DW0
  uint32_t Reserved; //DW1
  CMD_DWORD_2 DW2;
  CMD_DWORD_3 DW3;
}COMMAND_CQE, *PCOMMAND_CQE;

#pragma pack(pop)

typedef struct _NVME_DEVICE
{
  uintptr_t BAR0_VA;
  uint8_t DSTRD;

  PCOMMON_CMD ASQ_VA;
  uint64_t ASQ_PA;
  uint16_t SQ_TAIL;

  PCOMMAND_CQE ACQ_VA;
  uint64_t ACQ_PA;
  uint16_t CQ_HEAD;
  uint8_t CQ_PHASE;

  volatile uint32_t* ASQ_DB;
  volatile uint32_t* ACQ_DB; 
}NVME_DEVICE, *PNVME_DEVICE;

typedef struct _NVME_IO_QUEUE
{
  PCOMMON_CMD IOSQ_VA;
  uint64_t IOSQ_PA;
  uint16_t SQ_TAIL;

  PCOMMAND_CQE IOCQ_VA;
  uint64_t IOCQ_PA;
  uint16_t CQ_HEAD;
  uint8_t CQ_PHASE;

  volatile uint32_t *SQ_DB;
  volatile uint32_t *CQ_DB;
}NVME_IO_QUEUE, *PNVME_IO_QUEUE;

static inline volatile uint32_t* NvmeGetSQ(uintptr_t bar0_va, uint16_t qid, uint8_t dstrd)
{
  uint32_t stride = 4 << dstrd;
  uintptr_t offset = 0x1000 + (qid << 1) * stride;
  return (volatile uint32_t *)(bar0_va + offset);
}

static inline volatile uint32_t* NvmeGetCQ(uintptr_t bar0_va, uint16_t qid, uint8_t dstrd)
{
  uint32_t stride = 4 << dstrd;
  uintptr_t offset = 0x1000 + ((qid << 1) + 1) * stride;
  return (volatile uint32_t *)(bar0_va + offset);
}

uint8_t PciScanNvme(PPCI_DEVICE device);
uint8_t NvmeInit(uint64_t bar0_pa);
uint16_t NvmeAdminCommand(PCOMMON_CMD cmd, PCOMMAND_CQE cqe_o);
uint16_t NvmeIoCommand(PCOMMON_CMD cmd);
uint8_t NvmeInitIoQueues(void);

uint16_t NvmeRead(uint64_t lba, uint64_t sectors_count, uint64_t buffer_pa);
uint16_t NvmeWrite(uint64_t lba, uint64_t sectors_count, uint64_t buffer_pa);

void NvmeIdentifyTest(void);
void NvmeReadWriteTest(void);
#endif
