//
//  main.cpp --
//
//  $DateTime: 2006/10/24 23:40:45 $
//
#include <stdio.h>

#ifndef ___osiBus_h___
 #include "osiBus.h"
#endif 

void initMite(iBus *bus);			//Initialise Mite Chip.
void test(iBus *bus);				//test function contains the functionality

int main()
{
	iBus* bus;

    bus = acquireBoard("PXI8::13::INSTR");
    
    if(bus == NULL){
		printf("Could not access PCI device.  Exiting.\n");
		return 1;
	}
        
	//Intitialise Mite Chip.
	initMite(bus);

	//Calling test function
	test(bus);

	releaseBoard(bus);
	return 0;
}


void initMite(iBus *bus)
{
	tAddressSpace  bar0;
	u32 physicalBar1;
	//void* physicalBar1; //64bit support devmem
	
	//Skip MITE initialization for PCMCIA boards
	//(which do not have a MITE DMA controller)
	if(!bus->get(kIsPciPxiBus,0)) return;
	
	bar0 = bus->createAddressSpace(kPCI_BAR0);

	//Get the physical address of the DAQ board
	physicalBar1 = bus->get(kBusAddressPhysical,kPCI_BAR1);
	//Tell the MITE to enable BAR1, where the rest of the board's registers are
	bar0.write32(0xC0, (physicalBar1 & 0xffffff00L) | 0x80);
	//bar0.write32(0xC0, ((unsigned int)((unsigned long)physicalBar1) & 0xffffff00L) | 0x80); //64bit support devmem
	
	bus->destroyAddressSpace(bar0);
}
