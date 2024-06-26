//
// initialize6723.cpp
// 
// Initialize 6723 fpga images
//
//  $DateTime: 2009/04/09 10:33:19 $
//

#ifndef ___fpga6723_h___
 #include "fpga6723.h"
#endif

#define bAddressCPLD		0
#define FPGA			1

// from FPGA_6723.cpp
extern unsigned int FPGA_6723_sizeInBytes;
extern unsigned char FPGA_6723_image[];

void test(iBus *bus)
{
  u32 IO = configMite(bus);
  programFPGA(bus, bAddressCPLD, FPGA, FPGA_6723_image, FPGA_6723_sizeInBytes);
  configMite_io(bus, IO);
}
