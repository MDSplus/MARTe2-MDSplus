//
//  aoex1.cpp --
//

#include <stdio.h>

#ifndef ___t6723_h___
    #include "t6723.h"
#endif

void test(iBus *bus)
{
  tAddressSpace  bar1;
  t6723 *board;

  bar1 = bus->createAddressSpace(kPCI_BAR1);
  board = new t6723(bar1);
  
  //Check whether FPGA downloaded correctly
  u32 check = 0, version = 0;
  version = board->FPGAVersion.read();
  printf("Expected FPGA version: 04111813 --- Actual FPGA version: %08X\n", version);
  check = board->FPGACheck.read();
  printf("Expected FPGA check: 36373233 --- Actual FPGA check = %X\n", check);

  //single point ouput 5 V on AO channel 0
  board->AOMisc.writeAOEnable(1);
  board->AOMisc.writeAOUpdateMode(t6723::tAOMisc::kAOUpdateModeImmediate);
  board->DirectData_00.write(0x0FFF);
  
  delete board;
  bus->destroyAddressSpace(bar1);
}
