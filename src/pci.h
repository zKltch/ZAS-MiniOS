#ifndef PCI_H
#define PCI_H

#include <stdint.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

#pragma pack(push, 1)
typedef union _CONFIG_ADDRESS
{
  struct
  {
    uint32_t Register_Offset : 8;
    uint32_t Function_Number : 3;
    uint32_t Device_Number : 5;
    uint32_t Bus_Number : 8;
    uint32_t Reserved : 7;
    uint32_t Enable_Bit : 1;
  };
  uint32_t Flag;
}CONFIG_ADDRESS, *PCONFIG_ADDRESS;

typedef union _COMMAND_REGISTER
{
  struct{
    uint16_t IO_Space : 1;
    uint16_t Memory_Space : 1;
    uint16_t Bus_Master : 1;
    uint16_t Special_Cycles : 1;
    uint16_t MemW_Inv_Enable : 1; //Memory Write and Invalidate Enable
    uint16_t VGA_Palette_Snoop : 1;
    uint16_t Parity_Error_Response : 1;
    uint16_t Reserved1 : 1;
    uint16_t SERR_Enable : 1;
    uint16_t Fast_B2b_Enable : 1; //Back-to-Back
    uint16_t INT_Disable : 1;
    uint16_t Reserved2 : 5;
  };
  uint16_t Flag;
}COMMAND_REGISTER, *PCOMMAND_REGISTER;

typedef union _STATUS_REGISTER
{
  struct
  {
    uint16_t Reserved1 : 3;
    uint16_t INT_Status : 1;
    uint16_t Capabilities_List : 1;
    uint16_t MHz_66_Capable : 1;
    uint16_t Reserved2 : 1;
    uint16_t Fast_Btb_Capable : 1;
    uint16_t Master_Data_Parity_Error : 1;
    uint16_t DEVSEL_Timing : 1;
    uint16_t Signaled_Target_Abort : 1;
    uint16_t Received_Target_Abort : 1;
    uint16_t Received_Master_Abort : 1;
    uint16_t Signaled_System_Error : 1;
    uint16_t Detected_Parity_Error : 1;
  };
  uint16_t Flag;
}STATUS_REGISTER, *PSTATUS_REGISTER;

typedef union _HEADER_TYPE_REGISTER
{
  struct
  {
    uint8_t HeaderType : 7;
    uint8_t MF : 1;
  };
  uint8_t Value;
}HEADER_TYPE_REGISTER, *PHEADER_TYPE_REGISTER;

typedef union _BIST_REGISTER
{
  struct
  {
    uint8_t Completion_Code : 4;
    uint8_t Reserved : 2;
    uint8_t StartBIST : 1;
    uint8_t BIST_Capable : 1;
  };
  uint8_t Value;
}BIST_REGISTER, *PBIST_REGISTER;

typedef union _BASE_ADDRESS_REGISTER
{
  struct
  {
    uint32_t Always_0 : 1;
    uint32_t Type : 2;
    uint32_t Prefetchable : 1;
    uint32_t Aligned_BA_16B : 28;
  };
  uint32_t Value;
}BASE_ADDRESS_REGISTER, *PBASE_ADDRESS_REGISTER;

typedef union _IO_SPACE_BAR
{
  struct
  {
    uint32_t Always1 : 1;
    uint32_t Reserved : 1;
    uint32_t Aligned_BA_4B : 30;
  };
  uint32_t Value;
}IO_SPACE_BAR, *PIO_SPACE_BAR;

typedef union _BA_REGISTER
{
  BASE_ADDRESS_REGISTER BAR;
  IO_SPACE_BAR IO_BAR;
}BA_REGISTER, *PBA_REGISTER;

#pragma pack(pop)

typedef union _HEADER_FIELD
{
  struct
  {
    uint16_t VendorID;
    uint16_t DeviceID;
  };
  struct
  {
    COMMAND_REGISTER Command;
    STATUS_REGISTER Status;
  };
  struct
  {
    uint8_t RevisionID;
    uint8_t ProgIF;
    uint8_t SubClass;
    uint8_t ClassCode;
  };
  struct
  {
    uint8_t CacheLineSize;
    uint8_t LatencyTimer;
    HEADER_TYPE_REGISTER HeaderType;
    BIST_REGISTER BIST;
  };

  union _HEADER_TYPE_0
  {
    BA_REGISTER BAR0; //Base Address
    BA_REGISTER BAR1;
    BA_REGISTER BAR2;
    BA_REGISTER BAR3;
    BA_REGISTER BAR4;
    BA_REGISTER BAR5;
    uint32_t CardBus_CIS_Pointer;
    struct
    {
      uint16_t SubsystemVendorID;
      uint16_t SubsystemID;
    };
    uint32_t ExROM_BA;
    struct
    {
      uint8_t Capabilities_Pointer;
      uint8_t Reserved1;
      uint16_t Reserved2;
    };
    uint32_t Reserved3;
    struct
    {
      uint8_t INT_Line;
      uint8_t INT_PIN;
      uint8_t MIN_Grant;
      uint8_t MAX_Latency;
    };
  }HEADER_TYPE_0;


  union _HEADER_TYPE_1
  {
    BA_REGISTER BAR0;
    BA_REGISTER BAR1;
    struct
    {
      uint8_t Primary_Bus_Number;
      uint8_t Secondary_Bus_Number;
      uint8_t Subordinate_Bus_Number;
      uint8_t Secondary_Latency_Timer;
    };
    struct
    {
      uint8_t IO_Base;
      uint8_t IO_Limit;
      uint16_t Secondary_Status;
    };
    struct
    {
      uint16_t Memory_Base;
      uint16_t Memory_Limit;
    };
    struct
    {
      uint16_t Prefetchable_Memory_Base;
      uint16_t Prefetchable_Memory_Limit;
    };
    uint32_t Prefetch_Base_Upper;
    uint32_t Prefetch_Limit_Upper;
    struct
    {
      uint16_t IO_Base_Upper;
      uint16_t IO_Limit_Upper;
    };
    struct
    {
      uint8_t Capability_Pointer;
      uint8_t Reserved1;
      uint16_t Reserved2;
    };
    uint32_t ExROM_BA;
    struct
    {
      uint8_t INT_Line;
      uint8_t INT_PIN;
      uint16_t Bridge_Control;
    };
  }HEADER_TYPE_1;

  union _HEADER_TYPE_2
  {
    uint32_t CardBus_Socket_ExCa_BA;
    struct
    {
      uint8_t Offset_Of_Capabilities_List;
      uint8_t Reserved;
      uint16_t Secondary_Status;
    };
    struct
    {
      uint8_t PCI_Bus_Number;
      uint8_t CardBus_Bus_Number;
      uint8_t Subordinate_Bus_Number;
      uint8_t CardBus_Latency_Timer;
    };
    uint32_t Memory_BA_0;
    uint32_t Memory_Limit_0;
    uint32_t Memory_BA_1;
    uint32_t Memory_Limit_1;
    uint32_t IO_BA_0;
    uint32_t IO_Limit_0;
    uint32_t IO_BA_1;
    uint32_t IO_Limit_1;
    struct
    {
      uint8_t INT_Line;
      uint8_t INT_PIN;
      uint16_t Bridge_Control;
    };
    struct
    {
      uint16_t Subsystem_Device_ID;
      uint16_t Subsystem_Vendor_ID;
    };
    uint32_t PC_Card_Legacy_Mode_BA_16b;
  }HEADER_TYPE_2;

  uint32_t Value;
}HEADER_FIELD, *PHEADER_FIELD;

/*
https://wiki.osdev.org/PCI
Class Code
Subclass
Prog IF
*/

typedef struct _PCI_DEVICE
{
  uint8_t Bus;
  uint8_t Device;
  uint8_t Function;
  uint16_t VendorID;
  uint16_t DeviceID;
  uint8_t ClassCode;
  uint8_t SubClass;
  uint8_t ProgIF;
  //used for Nvme
  uint64_t BAR0;
  uint32_t BAR0_SIZE;
}PCI_DEVICE, *PPCI_DEVICE;

static inline void outl(uint16_t port, uint32_t value)
{
  asm volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint32_t inl(uint16_t port)
{
  uint32_t value;
  asm volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
  return value;
}

uint32_t PciConfigRead_DWORD(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset);
void PciConfigWrite_DWORD(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint32_t val);
void PciEnableFeatures(uint8_t bus, uint8_t dev, uint8_t func);
uint64_t PciGetBAR(uint8_t bus, uint8_t dev, uint8_t func);
uint8_t PciScanNvme(PPCI_DEVICE device);

#endif
