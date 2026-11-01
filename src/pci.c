#include "pci.h"

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

uint64_t PciGetBAR0(uint8_t bus, uint8_t dev, uint8_t func)
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
