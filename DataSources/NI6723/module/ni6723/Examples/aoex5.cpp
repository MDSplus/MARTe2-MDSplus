//
//  aoex5.cpp 
//
//  This program outputs a sawtooth for each consecutive channel in numChans.
//  The length of the sawtooth is based on the number of channels and the 
//  numberOfBuffers to output.

#include <stdio.h>

#ifndef ___t6723_h___
	#include "t6723.h"
#endif

#ifndef ___tSTC_h___
	#include "tSTC.h"
#endif

#ifndef ___common_h___
 #include "common.h"
#endif

#ifndef ___ao_h___
 #include "ao.h"
#endif 

#define MaxReading 10
#define FIFOSize 2047

void test(iBus *bus)
{
	tAddressSpace  bar1;
	t6723 *board;
	tSTC *theSTC;

	u16 numChans = 1;

	const tBoolean isContinuous = kTrue; 
	const u32 numberOfBuffers = 10; // 24-bit value
	const u32 numberOfSamplesPerChan = 2047;
	const u32 numberOfSamplesTotal =  numChans * numberOfSamplesPerChan;

	if(numberOfSamplesTotal > FIFOSize)
	{
		printf("Total number of samples to be generated is greater than FIFO size.  Exiting...\n");
		return;
	}

	float voltage; // tmp variable for generating wfm 
	int voltArray[numberOfSamplesTotal];	// array of voltages to output 
	unsigned long data_index; // counter to count number of points 
	unsigned long fifo_full=0; // flag to indicate data FIFO full 

	bar1 = bus->createAddressSpace(kPCI_BAR1);
	board = new t6723(bar1);
	theSTC = new tSTC(bar1);

	// Return if FPGA check is invalid
	if ( !(FPGA_Check(board, 0x36373233)) )
	{
		printf("FPGA_Check failed.  Exiting...\n");
		return;
	}

	//Read Calibration Constants from EEPROM
	double gainDbl[32];
	double offsetDbl[32];
	u16 address = 0;
	u16 rawOut = 0;

	// Read calibration constants address from EEPROM in 0x18 and 0x19. Add 0x10 for header info.
	u16 calBase = ((u16) readFromEEPROM(board, 0x18) << 8 | readFromEEPROM(board, 0x19)) + 0x10;
	address = calBase;

	// Read calibration constants for all 32 channels of the 6723 and store in gain and offset arrays
	for (int j = 0; j < 32; j++)
	{
		int i;
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

	//Reset DAQ-STC  
	aoReset(theSTC);

	//Configure DAQ-STC for a AO operation
	aoPersonalize(theSTC);

	//Now Program the DAQ-STC
	//Configure the timebase options for DAQ-STC 
	aoClock(theSTC);

	// Enable DAC outputs and set to waveform mode
	//board->AOMisc.setAOCalDisable(1);
	board->AOMisc.setAOEnable(1);
	board->AOMisc.setAOUpdateMode(board->AOMisc.kAOUpdateModeWaveform);
	board->AOMisc.flush();
	
	// Values written to AOWaveFormGeneration dictate the order of
	// generation for the AO DACs.  For this example we choose an 
	// update order that increases sequentially.
	board->AOChanCount.writeRegister(numChans);
	for(int i=0; i < numChans; i++)
		board->AOWaveformGeneration.writeRegister(i);


	//Load voltage array with values to be output. 
	//The waveform in this example ramps linearly from the minimum voltage 
	//to the maximum voltage, as determined by MaxReading. 
	//A ramp is generated for each channel that is used.
	int j = 0;
	for (int i=0; i < numberOfSamplesPerChan; i+=numChans) 
	{
		//ramp from smallest voltage to largest voltage 
		voltage = i * 2.0F * MaxReading / (numberOfSamplesPerChan - 1.0F) - MaxReading;

		j=0;
		while((j<numChans) && ((j+i)<numberOfSamplesPerChan))
		{
			u16 rawOut = 0;
			double corrected = voltage * gainDbl[j] + offsetDbl[j];
			double countsPerVolt = 8192.0 / 20.0;  // 13-bits over -10V..+10V
			double rounder;
			if ( corrected >= 0 ) {
				rounder = 0.5;
			} else {
				rounder = -0.5;
			}
			rawOut = unsigned(countsPerVolt * corrected + rounder);

			voltArray[(i * numChans) + j] = (int)rawOut;
			j++;
		}
	}
	
	//Reset the AO data FIFO
	aoClearFifo(theSTC);

	//Pre-load the data FIFO with the voltages from the array
	data_index = 0L;
	while(!fifo_full && (data_index < (unsigned long)numberOfSamplesTotal))
	{
		//Load data into the data FIFO.
		board->FIFOData.writeRegister((unsigned)voltArray[data_index++]);

		//Check to see if the data FIFO is full.
		fifo_full = board->DACFIFOFlags.readRegister();
		fifo_full = fifo_full & 0x0004;
	}

	// Setup the Start Trigger
	aoStartTrigger(theSTC, theSTC->AO_Trigger_Select.kAO_START1_SelectPFI0, theSTC->AO_Trigger_Select.kAO_START1_PolarityActive_High);

  	aoCount(theSTC, numberOfSamplesTotal, numberOfBuffers, isContinuous );

	aoUpdate(theSTC, 4000 /* 20M/<desired update rate> */);

	aoChannelSelect(theSTC);

	//Set the error conditions upon which the AOTM will stop.
	aoStop(theSTC, isContinuous);

	aoFifoMode(theSTC, 1);

	aoInterrupt(theSTC);

	//Arm the counters and preload the DAC with the first analog output value
	aoArm(theSTC);

	//Start the generation
	aoStart(theSTC);

	//Wait until the generation has completed
	if (aoComplete(theSTC, isContinuous) == 1)
	{
		printf("error FIFO overrun\n");
	}
	else
	{
		printf("generation complete\n");
	}

	aoDisarm(theSTC);

	aoClearFifo(theSTC);

	printf("%d Points Written\n", data_index);
	delete theSTC;
	delete board;
	bus->destroyAddressSpace(bar1);
}
