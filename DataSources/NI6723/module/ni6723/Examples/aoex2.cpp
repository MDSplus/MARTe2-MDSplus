//
//  aoex2.cpp
//  This example demonstrates how to query the device's firmware and internal
//  calibration information
//

#include <stdio.h>
#include <math.h>

#ifndef ___t6723_h___
  #include "t6723.h"
#endif

#ifndef ___common_h___
 #include "common.h"
#endif

void test(iBus *bus)
{
  tAddressSpace  bar1;
  t6723 *board;
  
  bar1 = bus->createAddressSpace(kPCI_BAR1);
  board = new t6723(bar1);
  
  // Check whether FPGA downloaded correctly
  u32 check = 0, version = 0;
  version = board->FPGAVersion.read();
  printf("Expected FPGA version: 04111813 --- Actual FPGA version: %08X\n", version);
  check = board->FPGACheck.read();
  printf("Expected FPGA check: 36373233 --- Actual FPGA check = %X\n", check);
  
  if ( version != 0x4111813 || check != 0x36373233 ) {
    printf("FPGA version check failed. Please verify that it has been properly\nloaded with initialize6723.\n");
  }
  
  u32 gainInt = 0;
  u32 offsetInt = 0;
  double gainDbl = 0;
  double offsetDbl = 0;
  u16 address = 0;
  int i;
  
  // Read calibration constants address from EEPROM in 0x18 and 0x19. Add 0x10 for header info.
  u16 calBase = ((u16) readFromEEPROM(board, 0x18) << 8 | readFromEEPROM(board, 0x19)) + 0x10;

  printf("calBase: %x\n", calBase);

  u16 rawOut = 0;
  double out = 10.0;
  
  printf("Channel 0 gain correction\n");
  address = calBase;
  for (i=0;i<4;i++)
    gainInt = (gainInt << 8) + readFromEEPROM(board, address + i);
  
  gainDbl = u32ToDouble(gainInt);
  printf("  %f (0x%.4X)\n", gainDbl, gainInt);
  
  printf("Channel 0 offset correction\n");
  address = calBase + 4;
  for (i=0;i<4;i++)
    offsetInt = (offsetInt << 8) + readFromEEPROM(board, address + i);
  
  offsetDbl = u32ToDouble(offsetInt);
  printf("  %f (0x%.4X)\n", offsetDbl, offsetInt);
  
  // Single point ouput on AO channel 0
  board->AOMisc.writeAOEnable(1);
  board->AOMisc.writeAOUpdateMode(t6723::tAOMisc::kAOUpdateModeImmediate);
  
  double corrected = out * gainDbl + offsetDbl;
  double countsPerVolt = 8192.0 / 20.0;  // 13-bits over -10V..+10V
  double rounder;
  if ( corrected >= 0 ) {
    rounder = 0.5;
  } else {
    rounder = -0.5;
  }
  rawOut = unsigned(countsPerVolt * corrected + rounder);
  
  printf("rawOut = %d out = %f (corrected to %f)\n", rawOut, out, corrected);
  board->DirectData_00.write(rawOut);
  
  delete board;
  bus->destroyAddressSpace(bar1);
}

