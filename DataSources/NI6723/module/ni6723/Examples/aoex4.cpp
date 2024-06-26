//
//  aoex2.cpp
//  This example demonstrates how to query the device's firmware and internal
//  calibration information
//

#include <stdio.h>
#include <math.h>
#include <unistd.h>

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

  double gainDbl[32];
  double offsetDbl[32];
  u16 address = 0;
  int i, j;
  u16 calBase = 0x64;
  u16 rawOut = 0;
  double out = 0;
  
  // Check whether FPGA downloaded correctly
  u32 check = 0, version = 0;
  version = board->FPGAVersion.read();
  printf("Expected FPGA version: 04111813 --- Actual FPGA version: %08X\n", version);
  check = board->FPGACheck.read();
  printf("Expected FPGA check: 36373233 --- Actual FPGA check = %X\n", check);
  
  if ( version != 0x4111813 || check != 0x36373233 ) {
    printf("FPGA version check failed. Please verify that it has been properly\nloaded with initialize6723.\n");
    goto error;
  }
  
  
  address = calBase;

  for (j = 0; j < 32; j++)
  {
    u32 gainInt = 0;
    u32 offsetInt = 0;

    for (i=0;i<4;i++)
      gainInt = (gainInt << 8) + readFromEEPROM(board, address + i);
  
    gainDbl[j] = u32ToDouble(gainInt);

    address += 4;
    for (i=0;i<4;i++)
      offsetInt = (offsetInt << 8) + readFromEEPROM(board, address + i);
  
    offsetDbl[j] = u32ToDouble(offsetInt);
    address += 4;
  }
  
  // Single point ouput on AO channel 0
  board->AOMisc.writeAOEnable(1);
  board->AOMisc.writeAOUpdateMode(t6723::tAOMisc::kAOUpdateModeImmediate);


  for (out = 0.0; out <= 10.0; out+=0.05)
  {
	printf("\rOutput Value = %f   ", out);
	fflush( stdout );
	for (j = 0; j < 32; j++)
	{
	  double corrected = out * gainDbl[j] + offsetDbl[j];
	  double countsPerVolt = 8192.0 / 20.0;  // 13-bits over -10V..+10V
	  double rounder;
	  if ( corrected >= 0 ) {
	    rounder = 0.5;
	  } else {
	    rounder = -0.5;
	  }
	  rawOut = unsigned(countsPerVolt * corrected + rounder);
  
//	  printf("CH %d - rawOut = %d out = %f (corrected to %f)\n", j, rawOut, out, corrected);
	  switch (j)
	  {
		case 0: board->DirectData_00.write(rawOut); break;
		case 1: board->DirectData_01.write(rawOut); break;
		case 2: board->DirectData_02.write(rawOut); break;
		case 3: board->DirectData_03.write(rawOut); break;
		case 4: board->DirectData_04.write(rawOut); break;
		case 5: board->DirectData_05.write(rawOut); break;
		case 6: board->DirectData_06.write(rawOut); break;
		case 7: board->DirectData_07.write(rawOut); break;
		case 8: board->DirectData_08.write(rawOut); break;
		case 9: board->DirectData_09.write(rawOut); break;
		case 10: board->DirectData_10.write(rawOut); break;
		case 11: board->DirectData_11.write(rawOut); break;
		case 12: board->DirectData_12.write(rawOut); break;
		case 13: board->DirectData_13.write(rawOut); break;
		case 14: board->DirectData_14.write(rawOut); break;
		case 15: board->DirectData_15.write(rawOut); break;
		case 16: board->DirectData_16.write(rawOut); break;
		case 17: board->DirectData_17.write(rawOut); break;
		case 18: board->DirectData_18.write(rawOut); break;
		case 19: board->DirectData_19.write(rawOut); break;
		case 20: board->DirectData_20.write(rawOut); break;
		case 21: board->DirectData_21.write(rawOut); break;
		case 22: board->DirectData_22.write(rawOut); break;
		case 23: board->DirectData_23.write(rawOut); break;
		case 24: board->DirectData_24.write(rawOut); break;
		case 25: board->DirectData_25.write(rawOut); break;
		case 26: board->DirectData_26.write(rawOut); break;
		case 27: board->DirectData_27.write(rawOut); break;
		case 28: board->DirectData_28.write(rawOut); break;
		case 29: board->DirectData_29.write(rawOut); break;
		case 30: board->DirectData_30.write(rawOut); break;
		case 31: board->DirectData_31.write(rawOut); break;
		default: break;
	  }
	  
	}
	  fflush( stdout );

	  usleep(100000);
  }
error:
  delete board;
  bus->destroyAddressSpace(bar1);
}

