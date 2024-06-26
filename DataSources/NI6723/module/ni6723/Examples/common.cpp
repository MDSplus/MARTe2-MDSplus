//
//  common.cpp
//
//  $DateTime: 2010/08/12 14:41:45 $
//
#include <math.h>
#include "common.h"

// Call this function to check whether FPGA downloaded correctly
bool FPGA_Check(t6723 *board, u32 expectedCheck)
{	  
	u32 check = 0, version = 0;
	
	version = board->FPGAVersion.read();
	
	check = board->FPGACheck.read();

	return (check == expectedCheck);
}

void spin()
{
	for(int i=0;i<10000;i++)
	{}
	return;
}

u8 readFromEEPROM(t6723 *board, u16 address)
{
	u8 value = 0;
	u32 command = 0;
	int i;

	for(i=0;i<16;i++)
		command = (command<<1) + ((address>>i) & 1);
	command = (command<<8) + 0xC0;

	//initialize
	board->SerialCommand.writeSerialClock((u8)0);
	board->SerialCommand.writeSerialData((u8)0);
	board->SerialCommand.writeEEPromChipSelect((u8)0);
	board->SerialCommand.writeMiscControlLines((u8)0);
	board->SerialCommand.writeEEPromChipSelect((u8)1);

	//write command including address
	for(i=0;i<24;i++)
	{
		board->SerialCommand.writeSerialData((u8)((command>>(i)) & 1));
		spin();
		board->SerialCommand.writeSerialClock((u8)0);
		spin();
		board->SerialCommand.writeSerialClock((u8)1);
		spin();
	}

	//read response from EEPROM
	for(i=0;i<8;i++)
	{
		board->SerialCommand.writeSerialClock((u8)1);
		spin();
		board->SerialCommand.writeSerialClock((u8)0);
		spin();
		value = (value<<1) + (board->StatusReg.readPROMOUT());
	}

	//de-initialize
	board->SerialCommand.writeSerialClock((u8)0);
	board->SerialCommand.writeSerialData((u8)0);
	board->SerialCommand.writeEEPromChipSelect((u8)0);

	return value;
}

double u32ToDouble(u32 value)
{
	u32 signBit = 0;
	u32 exp = 0;
	int exponent = 0;
	u32 significand = 0;
	int i = 0;
	double temp = 1;
	double result;

	signBit = (value & 0x80000000);
	exp = (value & 0x7F800000) >> 23 ;
	exponent = (int)exp - 127;
	significand = (value & 0x007FFFFF);

	for (i=0;i<23;i++) {
		if (significand & (int)pow((double)2,i) )
		{
			temp+=( 1 / pow((double)2,(23-i)) );
		}
	}
	result = pow( (double)2, exponent) * temp;
	if(signBit) result = -result;

	return result;
}
