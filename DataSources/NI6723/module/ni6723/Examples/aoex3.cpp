//
//  aoex3.cpp 
//
//  This program outputs a sawtooth for each consecutive channel in numChans.
//  The length of the sawtooth is based on the number of channels and the 
//  numberOfBuffers to output.

#include <stdio.h>
#include <math.h>

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


#define MaxReading 2000
#define FIFOSize 2047

void test(iBus *bus)
{
	tAddressSpace  bar1;
	t6723 *board;
	tSTC *theSTC;

	u16 numChans = 1;

	const tBoolean isContinuous = kFalse; 
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

	//Reset DAQ-STC  
	aoReset(theSTC);

	//Configure DAQ-STC for a AO operation
	aoPersonalize(theSTC);

	//Now Program the DAQ-STC
	//Configure the timebase options for DAQ-STC 
	aoClock(theSTC);

	// Enable DAC outputs and set to waveform mode
	board->AOMisc.setAOCalDisable(1);
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
	for (int i=0; i < numberOfSamplesTotal; i+=numChans) 
	{
		//ramp from smallest voltage to largest voltage 
		voltage = i * 2.0F * MaxReading / (numberOfSamplesTotal - 1.0F) - MaxReading;
		j=0;
		while((j<numChans) && ((j+i)<numberOfSamplesTotal))
		{
			voltArray[i+j] = (int)voltage;
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


	aoStartTrigger(theSTC, theSTC->AO_Trigger_Select.kAO_START1_SelectPulse, theSTC->AO_Trigger_Select.kAO_START1_PolarityActive_High);

  	aoCount(theSTC, isContinuous, numberOfSamplesTotal, numberOfBuffers );

	aoUpdate(theSTC, 4000 /* 20M/<desired update rate> */);

	aoChannelSelect(theSTC);

	//Set the error conditions upon which the AOTM will stop.
	aoStop(theSTC, isContinuous);

	aoFifoMode(theSTC, 1);

	aoInterrupt(theSTC);

	//Arm the counters and preload the DAC with the first analog output value
	aoArm(theSTC);

	//Start the acquisition
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
