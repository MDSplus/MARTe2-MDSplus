//
//  main.cpp
//
//  $DateTime: 2009/04/09 10:33:19 $
//
#include <stdio.h>

#ifndef ___osiBus_h___
 #include "osiBus.h"
#endif

void initMite(iBus *bus);        //Initialize Mite chip
void test(iBus *bus);            //test function contains the functionality

int main()
{
    iBus* bus;

//    bus = acquireBoard("PXI19::11::INSTR");
    bus = acquireBoard("PXI12::15::INSTR");

    if (bus == NULL) {
        printf("Could not access PCI device.  Exiting.\n");
        return 1;
    }

    //Initialize Mite Chip
    initMite(bus);

    //Call test function
    test(bus);

    releaseBoard(bus);

    return(0);
}

void initMite(iBus *bus)
{
    tAddressSpace bar0;
    u32 physicalBar1;

    //Skip Mite initialization for PCMCIA boards
    //    which do not have a Mite DMA controller
    if(!bus->get(kIsPciPxiBus,0)) return;

    bar0 = bus->createAddressSpace(kPCI_BAR0);

    //Get the physical address of the DAQ board
    physicalBar1 = bus->get(kBusAddressPhysical, kPCI_BAR1);

    //Tell the Mite to enable BAR1, where the rest of the board's registers are
    bar0.write32(0xC0, (physicalBar1 & 0xffffff00L) | 0x80);

    bus->destroyAddressSpace(bar0);
}
