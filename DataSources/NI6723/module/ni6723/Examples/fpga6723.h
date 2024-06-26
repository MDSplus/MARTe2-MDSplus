//
// fpga6723.h
//
//  $DateTime: 2009/04/09 10:33:19 $
//
#ifndef ___fpga6723_h___
#define ___fpga6723_h___

#include <assert.h>
#include <time.h>

#ifndef ___osiBus_h___
 #include "osiBus.h"
#endif

void configMite_io(iBus *bus, u32 IOPCR);

u32 configMite(iBus *bus);				//Configure Dust Mite

u32 peekU32(iBus *bus, u32 peekAddr);			//Peek the value at offset peekAddr on bar1

u8 reverseU8(u8 data);					//Reverse the bits in u8

void programFPGA(iBus *bus, u32 baseAddrCPLD, u16 numFPGA, u8 *image, u32 sizeInBytes);	//Program the FPGA

#endif // ___fpga6723_h___
