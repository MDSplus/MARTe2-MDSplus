//
// fpga.cpp
//
// program the FPGA on the 6723
//
//  $DateTime: 2009/04/09 10:33:19 $
//
// --------------------------------------------
#include "fpga6723.h"

#include <stdio.h>
#include <unistd.h> 
#define DMAC1			0x500
#define statusReg3Mask		0xFFF0 
#define statusReg3Value		0x0010

void programFPGA(iBus *bus, u32 baseAddrCPLD, u16 numFPGA, u8 *image, u32 sizeInBytes)
{

printf("PROGRAM FPGA\n");

  tAddressSpace  bar0, bar1;
      
  bar0 = bus->createAddressSpace(kPCI_BAR0);
  bar1 = bus->createAddressSpace(kPCI_BAR1);

  // Set up the DMA controller
  bar0.write32(0x00 + DMAC1, 0x80000000);
  bar0.write32(0x04 + DMAC1, 0x00);
  bar0.write32(0x08 + DMAC1, 0x180000);
  bar0.write32(0x0C + DMAC1, 0xE20500);
  bar0.write32(0x14 + DMAC1, 0xE70240);
  bar0.write32(0x10 + DMAC1, 0x00);
  bar0.write32(0x18 + DMAC1, 0x00);
  bar0.write32(0x00 + DMAC1, 0x01);
  
  u16 statusReg3;
  
  // Check CPLD
  bar1.write16(baseAddrCPLD, numFPGA);
  printf("SCRITTO %d\n", numFPGA);
  short xxx = bar1.read16(baseAddrCPLD);
  printf("LETTO %d\n", xxx);
  
  sleep(1);
  statusReg3 = bar1.read16(baseAddrCPLD + 0x08);
  printf("STATUSREG: %x, statusReg3 & statusReg3Mask:%x statusReg3Value: %x\n", statusReg3, statusReg3 & statusReg3Mask, statusReg3Value);
  
  
  if ((statusReg3 & statusReg3Mask) != statusReg3Value)
  {
    printf("CPLD not responding\n");
    bus->destroyAddressSpace(bar0);
    bus->destroyAddressSpace(bar1);
    return;
  }

  u8 byte;

	// 
	//  Download image
	//

	u8 *fpgaImagePtr = image;
	u32 remaining = sizeInBytes;
	printf("Downloading FPGA Image....\n");

  while (remaining != 0)
  {
		byte = *fpgaImagePtr;
    byte = reverseU8(byte);
    
		//Write the data to the FIFO
    bar0.write8(0x80 + DMAC1, byte);
    remaining--;
    fpgaImagePtr++;

    //Wait until there's room in the FIFO
    for (int i = 0;; i++)
    {
      if ((bar0.read32(0x40 + DMAC1) & 0xFF) <= 16)
				break;

      if (i >= 15000)
      {
				printf("Timeout programming FPGA\n");
				bus->destroyAddressSpace(bar0);
    		bus->destroyAddressSpace(bar1);
				return;
      }
    }
  }

  //Wait for the FIFO to complete its operation
  time_t past, next;
  time(&past);
  while (true)
  {
    time(&next);
    if (difftime(next, past) > 1)
			break;
  }
  
  u16 done_pin = 0;
  done_pin = bar1.read16(0x00);
  bar1.write16(0x00, 0x00);

  //Check for FPGA Done
  if (!done_pin)
  {
    printf("FPGA failed to program\n");
    bus->destroyAddressSpace(bar0);
    bus->destroyAddressSpace(bar1);
    return;
  }

  //Reset the FPGA and the STC
  bar1.write16(0x02, 0);
  bar1.write16(0x02, 0xFFFF);

  bus->destroyAddressSpace(bar0);
  bus->destroyAddressSpace(bar1);
}

u32 peekU32(iBus *bus, u32 peekAddr)
{
  tAddressSpace  bar1;
  u32 peekValue;
      
  bar1 = bus->createAddressSpace(kPCI_BAR1);
  peekValue = bar1.read32(peekAddr);

  bus->destroyAddressSpace(bar1);

  return peekValue;
}

u8 reverseU8(u8 data)
{
  u8 rev = data << 1;

  data >>= 1;
  for (int i = 0; i < sizeof(data) * 8 - 2; i++)
  {
    rev |= data & 1;
    rev <<= 1;
    data >>= 1;
  } 
  rev |= data;

  return rev;
}

u32 configMite(iBus *bus)
{
  tAddressSpace  bar0;
  u32 physicalBar1;
  u32 IOPCR;
 
  bar0 = bus->createAddressSpace(kPCI_BAR0);

  //Get the physical address of the DAQ board
  physicalBar1 = bus->get(kBusAddressPhysical,kPCI_BAR1);

  //Tell the MITE to enable BAR1, where the rest of the board's registers are
  bar0.write32(0xC4, (physicalBar1 & 0xffffff00) | 0x8B);

  //Open IO ram space
  bar0.write32(0xC0, 0);
  bar0.write32(0xF4, 0x40000000L);
  IOPCR = bar0.read32(0x470);
  bar0.write32(0x470, (IOPCR & 0xff3fffff) | 0x800000);

  bus->destroyAddressSpace(bar0);
  return IOPCR;
}

void configMite_io(iBus *bus, u32 IOPCR)
{
  tAddressSpace  bar0;
  u32 physicalBar1;
      
  bar0 = bus->createAddressSpace(kPCI_BAR0);

  //Get the physical address of the DAQ board
  physicalBar1 = bus->get(kBusAddressPhysical,kPCI_BAR1);

  //Open IO RAM space
  bar0.write32(0xC0, (physicalBar1 & 0xffffff00) | 0xB0);
  bar0.write32(0x470, IOPCR);

  bus->destroyAddressSpace(bar0);
}
