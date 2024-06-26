// 
//  ao.cpp 
//
//  $DateTime: 2010/08/12 14:41:45 $
//
#include "ao.h"

//Check status of output to see when generation is complete, or
//if any errors have occurred.
//
//For continuous operation, the conditions to exit this loop
//will never occur.  You may consider implementing a timer function
//or some other utility to stop the output cleanly.
int aoComplete(tSTC *theSTC, tBoolean isContinuous)
{
	tBoolean status = kFalse;
	int i = 0;

	//
	// Wait for generation to complete or FIFO error
	//
	while(1)
	{
		// Refresh AO_Status_1 soft-copy
		theSTC->AO_Status_1.refresh();
		
		//
		// Generation complete - check if NOT continuous
		//
		if (theSTC->AO_Status_1.getAO_BC_TC_St() && !isContinuous)
		{
			return 0;
			break;
		}
		
		//
		// FIFO Overrun error
		//    The overrun flag is set on the last point. If BC_TC is
		//    set, the generation completed sucessfully.
		//
		if (theSTC->AO_Status_1.getAO_Overrun_St())
		{
			// error FIFO overrun
			return 1;
			break;
		}

		i++;

	}//while
	
	return 0;
}

// Call this function to reset the DAQ-STC and Stop any activities in progress.
void aoReset(tSTC *theSTC)
{ 
	// Starting AO configuration
	theSTC->Joint_Reset.setAO_Configuration_Start(1);
	theSTC->Joint_Reset.setAO_Configuration_End(0);
	theSTC->Joint_Reset.flush();

	theSTC->AO_Command_1.writeAO_Disarm(1);//Disarm idle counters BC, UC and UI

	theSTC->AO_Personal.writeRegister(0);
	theSTC->AO_Command_1.writeRegister(0);
	theSTC->AO_Command_2.writeRegister(0);
	theSTC->AO_Mode_1.writeRegister(0);
	theSTC->AO_Mode_2.writeRegister(0);
	theSTC->AO_Output_Control.writeRegister(0);
	theSTC->AO_Mode_3.writeRegister(0);
	theSTC->AO_START_Select.writeRegister(0);
	theSTC->AO_Trigger_Select.writeRegister(0);

	theSTC->Interrupt_B_Enable.writeRegister(0);

	theSTC->AO_Personal.writeAO_BC_Source_Select(theSTC->AO_Personal.kAO_BC_Source_SelectUC_TC);

	theSTC->Interrupt_B_Ack.setAO_BC_TC_Trigger_Error_Confirm(1);
	theSTC->Interrupt_B_Ack.setAO_BC_TC_Error_Confirm(1);
	theSTC->Interrupt_B_Ack.setAO_UC_TC_Interrupt_Ack(1);
	theSTC->Interrupt_B_Ack.setAO_BC_TC_Interrupt_Ack(1);
	theSTC->Interrupt_B_Ack.setAO_START1_Interrupt_Ack(1);
	theSTC->Interrupt_B_Ack.setAO_UPDATE_Interrupt_Ack(1);
	theSTC->Interrupt_B_Ack.setAO_START_Interrupt_Ack(1);
	theSTC->Interrupt_B_Ack.setAO_STOP_Interrupt_Ack(1);
	theSTC->Interrupt_B_Ack.setAO_Error_Interrupt_Ack(1);
	theSTC->Interrupt_B_Ack.flush();

	// Ending AO configuration
	theSTC->Joint_Reset.setAO_Configuration_Start(0);
	theSTC->Joint_Reset.setAO_Configuration_End(1);
	theSTC->Joint_Reset.flush();
	return;
} 

// Call this function to select the various timebase options.
void aoClock(tSTC *theSTC)
{                         
	theSTC->Clock_and_FOUT.setSlow_Internal_Timebase(1);
	theSTC->Clock_and_FOUT.setSlow_Internal_Time_Divide_By_2(1);
	theSTC->Clock_and_FOUT.setClock_To_Board(1);
	theSTC->Clock_and_FOUT.setClock_To_Board_Divide_By_2(1);
	theSTC->Clock_and_FOUT.flush();
	return;
}

// Call this function to configure DAQ-STC for a AO operation
void aoPersonalize(tSTC *theSTC)
{	
	// Starting AO configuration
	theSTC->Joint_Reset.setAO_Configuration_Start(1);
	theSTC->Joint_Reset.setAO_Configuration_End(0);
	theSTC->Joint_Reset.flush();

	theSTC->AO_Personal.setAO_Number_Of_DAC_Packages(theSTC->AO_Personal.kAO_Number_Of_DAC_PackagesDual_DAC_mode);
	theSTC->AO_Personal.setAO_Fast_CPU(theSTC->AO_Personal.kAO_Fast_CPUSlow);
	theSTC->AO_Personal.setAO_TMRDACWR_Pulse_Width(theSTC->AO_Personal.kAO_TMRDACWR_Pulse_WidthAbout_2_TIMEBASE_Periods);
	theSTC->AO_Personal.setAO_FIFO_Flags_Polarity(theSTC->AO_Personal.kAO_FIFO_Flags_PolarityActive_Low);//
	theSTC->AO_Personal.setAO_FIFO_Enable(1);
	theSTC->AO_Personal.setAO_AOFREQ_Polarity(theSTC->AO_Personal.kAO_AOFREQ_PolarityActive_High);
	theSTC->AO_Personal.setAO_DMA_PIO_Control(theSTC->AO_Personal.kAO_DMA_PIO_ControlFIFO_Interface);
	theSTC->AO_Personal.setAO_UPDATE_Original_Pulse(theSTC->AO_Personal.kAO_UPDATE_Original_PulseMax_Of_UPDATE_Pulsewidth);
	theSTC->AO_Personal.setAO_UPDATE_Pulse_Timebase(theSTC->AO_Personal.kAO_UPDATE_Pulse_TimebaseSelect_By_PulseWidth);
	theSTC->AO_Personal.setAO_UPDATE_Pulse_Width(theSTC->AO_Personal.kAO_UPDATE_Pulse_WidthAbout_3_TIMEBASE_Periods);
	theSTC->AO_Personal.setAO_BC_Source_Select(theSTC->AO_Personal.kAO_BC_Source_SelectUC_TC);
	theSTC->AO_Personal.setAO_Interval_Buffer_Mode(0);
	theSTC->AO_Personal.setAO_UPDATE2_Original_Pulse(theSTC->AO_Personal.kAO_UPDATE2_Original_PulseMax_Of_UPDATE2_Pulsewidth);
	theSTC->AO_Personal.setAO_UPDATE2_Pulse_Timebase(theSTC->AO_Personal.kAO_UPDATE2_Pulse_TimebaseSelect_By_PulseWidth);
	theSTC->AO_Personal.setAO_UPDATE2_Pulse_Width(theSTC->AO_Personal.kAO_UPDATE2_Pulse_WidthAbout_3_TIMEBASE_Periods);
	
	theSTC->AO_Personal.flush();

	theSTC->Clock_and_FOUT.setAO_Source_Divide_By_2(0);
	theSTC->Clock_and_FOUT.setAO_Output_Divide_By_2(1);
	theSTC->Clock_and_FOUT.flush();

	theSTC->AO_Output_Control.writeAO_UPDATE_Output_Select(theSTC->AO_Output_Control.kAO_UPDATE_Output_SelectActive_Low );
	theSTC->AO_Output_Control.setAO_External_Gate_Polarity(1);

	theSTC->AO_START_Select.writeAO_AOFREQ_Enable(0);

	// Ending AO configuration
	theSTC->Joint_Reset.setAO_Configuration_Start(0);
	theSTC->Joint_Reset.setAO_Configuration_End(1);
	theSTC->Joint_Reset.flush();
	
	return;
}

void aoStartTrigger(tSTC *theSTC, 
                tSTC::tAO_Trigger_Select::tAO_START1_Select source,
                tSTC::tAO_Trigger_Select::tAO_START1_Polarity polarity)
{
	// Starting AO configuration
	theSTC->Joint_Reset.setAO_Configuration_Start(1);
	theSTC->Joint_Reset.setAO_Configuration_End(0);
	theSTC->Joint_Reset.flush();

	theSTC->AO_Mode_1.writeAO_Trigger_Once(1);

	theSTC->AO_Trigger_Select.setAO_START1_Select(source);
	theSTC->AO_Trigger_Select.setAO_START1_Polarity(polarity);
	theSTC->AO_Trigger_Select.setAO_START1_Edge(1);
	theSTC->AO_Trigger_Select.setAO_START1_Sync(1);
	theSTC->AO_Trigger_Select.flush();

	theSTC->AO_Trigger_Select.writeAO_Delayed_START1(0);

	theSTC->AO_Mode_3.writeAO_Trigger_Length(theSTC->AO_Mode_3.kAO_Trigger_LengthDA_START1);

	// Ending AO configuration
	theSTC->Joint_Reset.setAO_Configuration_Start(0);
	theSTC->Joint_Reset.setAO_Configuration_End(1);
	theSTC->Joint_Reset.flush();

	return;
}	

// Call this function to arm the counters and preload the DAC with the 
// first analog output value
void aoArm(tSTC *theSTC)
{ 
	unsigned busy=1;

	theSTC->AO_Mode_3.writeAO_Not_An_UPDATE(1);

	theSTC->AO_Mode_3.writeAO_Not_An_UPDATE(0);

	while(busy) 
	{
		//Reading from register Joint_Status_2_Register
		busy = theSTC->Joint_Status_2.readRegister();
		busy = busy & 0x1000;
	}

	theSTC->AO_Command_1.setAO_UI_Arm(1);
	theSTC->AO_Command_1.setAO_UC_Arm(1);
	theSTC->AO_Command_1.setAO_BC_Arm(1);
	theSTC->AO_Command_1.flush();
	return;
}  

// Call this function to start the generation.
void aoStart(tSTC *theSTC)
{ 
	theSTC->AO_Command_2.writeAO_START1_Pulse(1);
	return;
}

void aoCount(tSTC *theSTC, u32 numberOfSamples, u32 numberOfBuffers, tBoolean continuous)
{
	theSTC->Joint_Reset.setAO_Configuration_Start(1);
	theSTC->Joint_Reset.setAO_Configuration_End(0);
	theSTC->Joint_Reset.flush();
	
	if (continuous)
		theSTC->AO_Mode_1.setAO_Continuous(theSTC->AO_Mode_1.kAO_ContinuousIgnore_BC_TC);
	else
		theSTC->AO_Mode_1.setAO_Continuous(theSTC->AO_Mode_1.kAO_ContinuousStop_On_BC_TC);
	theSTC->AO_Mode_1.flush();
	
	theSTC->AO_Mode_2.writeAO_BC_Initial_Load_Source(0);
	theSTC->AO_BC_Load_A.writeRegister( numberOfBuffers - 1 );
	theSTC->AO_Command_1.writeAO_BC_Load(1);
	
	theSTC->AO_Mode_2.writeAO_UC_Initial_Load_Source(0);
	theSTC->AO_UC_Load_A.writeRegister( numberOfSamples); // number of updates in the last buffer
	theSTC->AO_Command_1.writeAO_UC_Load(1);
	theSTC->AO_UC_Load_A.writeRegister( numberOfSamples - 1); // number of updates in each buffer
	
	theSTC->Joint_Reset.setAO_Configuration_Start(0);
	theSTC->Joint_Reset.setAO_Configuration_End(1);
	theSTC->Joint_Reset.flush();
	return;
}

void aoConfigureDAC(tSTC *theSTC)
{
	theSTC->Joint_Reset.setAO_Configuration_Start(1);
	theSTC->Joint_Reset.setAO_Configuration_End(0);
	theSTC->Joint_Reset.flush();

	theSTC->AO_Command_1.setAO_LDAC0_Source_Select(0);
	theSTC->AO_Command_1.setAO_DAC0_Update_Mode(0);
	theSTC->AO_Command_1.setAO_LDAC1_Source_Select(0);
	theSTC->AO_Command_1.setAO_DAC1_Update_Mode(0);
	theSTC->AO_Command_1.flush();

	theSTC->Joint_Reset.setAO_Configuration_Start(0);
	theSTC->Joint_Reset.setAO_Configuration_End(1);
	theSTC->Joint_Reset.flush();
}

void aoFifoMode(tSTC *theSTC, tBoolean fifoRetransmit)
{
	theSTC->Joint_Reset.setAO_Configuration_Start(1);
	theSTC->Joint_Reset.setAO_Configuration_End(0);
	theSTC->Joint_Reset.flush();

	theSTC->AO_Mode_2.setAO_FIFO_Mode(theSTC->AO_Mode_2.kAO_FIFO_ModeLess_Than_Half_Full);
	theSTC->AO_Mode_2.setAO_FIFO_Retransmit_Enable(fifoRetransmit);
	theSTC->AO_Mode_2.flush();

	theSTC->Joint_Reset.setAO_Configuration_Start(0);
	theSTC->Joint_Reset.setAO_Configuration_End(1);
	theSTC->Joint_Reset.flush();
}

// Call this function to program the update interval.
// Update interval is specified in units of 20MHz ticks
void aoUpdate(tSTC *theSTC, u32 updateInterval)
{
	theSTC->Joint_Reset.setAO_Configuration_Start(1);
	theSTC->Joint_Reset.setAO_Configuration_End(0);
	theSTC->Joint_Reset.flush();

	theSTC->AO_Command_2.writeAO_BC_Gate_Enable(0);

	theSTC->AO_Mode_1.setAO_UPDATE_Source_Select(theSTC->AO_Mode_1.kAO_UPDATE_Source_SelectUI_TC);
	theSTC->AO_Mode_1.setAO_UPDATE_Source_Polarity(theSTC->AO_Mode_1.kAO_UPDATE_Source_PolarityRising_Edge);
	theSTC->AO_Mode_1.flush();	

	theSTC->AO_Mode_1.setAO_UI_Source_Select(theSTC->AO_Mode_1.kAO_UI_Source_SelectAO_IN_TIMEBASE1);
	theSTC->AO_Mode_1.setAO_UI_Source_Polarity(theSTC->AO_Mode_1. kAO_UI_Source_PolarityRising_Edge);
	theSTC->AO_Mode_1.flush();

	theSTC->AO_Mode_2.setAO_UI_Initial_Load_Source(theSTC->AO_Mode_2.kAO_UI_Initial_Load_SourceReg_A);
	theSTC->AO_Mode_2.setAO_UI_Reload_Mode(theSTC->AO_Mode_2.kAO_UI_Reload_ModeNo_Change);
	theSTC->AO_Mode_2.flush();

	theSTC->AO_UI_Load_A.writeRegister(updateInterval - 1);
	theSTC->AO_Command_1.writeAO_UI_Load(1);

	theSTC->Joint_Reset.setAO_Configuration_Start(0);
	theSTC->Joint_Reset.setAO_Configuration_End(1);
	theSTC->Joint_Reset.flush();
	return;
}
// This function sets up the channels for the STC 
void aoChannelSelect(tSTC *theSTC)
{
	theSTC->Joint_Reset.setAO_Configuration_Start(1);
	theSTC->Joint_Reset.setAO_Configuration_End(0);
	theSTC->Joint_Reset.flush();

	theSTC->AO_Mode_1.setAO_Multiple_Channels(1);
	theSTC->AO_Mode_1.flush();

	// For the 6723, the STC must ALWAYS be set to use 2 channels
	theSTC->AO_Output_Control.setAO_Number_Of_Channels(2);
	theSTC->AO_Output_Control.flush();

	theSTC->Joint_Reset.setAO_Configuration_Start(0);
	theSTC->Joint_Reset.setAO_Configuration_End(1);
	theSTC->Joint_Reset.flush();
}
// Call this function to set the error conditions upon which the AOTM will stop.
void aoStop(tSTC *theSTC, tBoolean isContinuous)
{
	theSTC->Joint_Reset.setAO_Configuration_Start(1);
	theSTC->Joint_Reset.setAO_Configuration_End(0);
	theSTC->Joint_Reset.flush();

	if(isContinuous)
		theSTC->AO_Mode_3.setAO_Stop_On_BC_TC_Error(0);
	else
		theSTC->AO_Mode_3.setAO_Stop_On_BC_TC_Error(1);

	theSTC->AO_Mode_3.setAO_Stop_On_BC_TC_Trigger_Error(1);
	theSTC->AO_Mode_3.setAO_Stop_On_Overrun_Error(1);
	theSTC->AO_Mode_3.flush();

	theSTC->Joint_Reset.setAO_Configuration_Start(0);
	theSTC->Joint_Reset.setAO_Configuration_End(1);
	theSTC->Joint_Reset.flush();
	return;
}

void aoInterrupt(tSTC *theSTC)
{
	theSTC->Joint_Reset.setAO_Configuration_Start(1);
	theSTC->Joint_Reset.setAO_Configuration_End(0);
	theSTC->Joint_Reset.flush();

	theSTC->Interrupt_B_Enable.setAO_UPDATE_Interrupt_Enable(0);
	theSTC->Interrupt_B_Enable.setAO_BC_TC_Interrupt_Enable(0);
	theSTC->Interrupt_B_Enable.setAO_UC_TC_Interrupt_Enable(0);	
	theSTC->Interrupt_B_Enable.setAO_START1_Interrupt_Enable(0);
	theSTC->Interrupt_B_Enable.setAO_Error_Interrupt_Enable(0);
	theSTC->Interrupt_B_Enable.setAO_START_Interrupt_Enable(0);
	theSTC->Interrupt_B_Enable.setAO_STOP_Interrupt_Enable(0);
	theSTC->Interrupt_B_Enable.setAO_FIFO_Interrupt_Enable(0);
	theSTC->Interrupt_B_Enable.flush();

	theSTC->Joint_Reset.setAO_Configuration_Start(0);
	theSTC->Joint_Reset.setAO_Configuration_End(1);
	theSTC->Joint_Reset.flush();

	return;
}

void aoDisarm(tSTC *theSTC)
{
	theSTC->Joint_Reset.setAO_Configuration_Start(1);
	theSTC->Joint_Reset.setAO_Configuration_End(0);
	theSTC->Joint_Reset.flush();

	theSTC->AO_Command_1.writeAO_Disarm(1);

	theSTC->Joint_Reset.setAO_Configuration_Start(0);
	theSTC->Joint_Reset.setAO_Configuration_End(1);
	theSTC->Joint_Reset.flush();

	return;
}

void aoClearFifo(tSTC *theSTC)
{
	theSTC->Write_Strobe_2.writeRegister(1);
	return;
}
