//
//  serialNumber.cpp --
//
//      example on how to read the serial number
//
//  $DateTime: 2006/10/24 23:40:45 $
//
#include <stdio.h>

#include "osiBus.h"
#include "scale.h"

void test(iBus *bus)
{
    u32 serialNumber; 

    serialNumReadMSeries(bus, &serialNumber);
   
    printf ("Serial Number: 0x%08X\n", serialNumber);
}