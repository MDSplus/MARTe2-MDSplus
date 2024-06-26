//
//  doiex1.cpp --
//
//      Digital read/write
//
//  $DateTime: 2006/10/24 23:40:45 $
//

#include <stdio.h>

#ifndef ___tMSeries_h___
 #include "tMSeries.h"
#endif

void test(iBus *bus)
{
    
    // create register map
    
	tAddressSpace  bar1;
	tMSeries *board;

	bar1 = bus->createAddressSpace(kPCI_BAR1);
	board = new tMSeries(bar1);
    
    // ---- Start DIO task ----
    board->DIO_Direction.writeRegister (0xFF);     
    

    // ---- Write to port ----
        
    board->Static_Digital_Output.writeRegister (0x55);
        
    // ---- Read-back value ----
        
    u8 value; 
    value = (u8) board->Static_Digital_Input.readRegister();
    
    printf ("port value: 0x%02X\n", value);

    
    //cleanup
    delete board;
	bus->destroyAddressSpace(bar1);
} 
