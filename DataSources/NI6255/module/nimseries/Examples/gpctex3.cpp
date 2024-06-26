//
//  gpctex3.cpp --
//
//      Period Measurement
//
//  $DateTime: 2006/10/24 23:40:45 $
//
//  Count pulses from an external source using an external gate using counter 0.
//
//  Connect an external source to pin 37 (PFI 8), and connect an external gate
//  to pin 3 (PFI 9)
//
//  For a countdown timer, make to following changes:
//  1. Comment out the delay section and uncomment the countdown timer secion in
//     the function test.
//  2. In the function simpleEventCountConfigGPCT0, change the following values:
//     a. G0_Source_Select to be one of the internal timebases (0, 18, or 30)
//     b. G0_Gate_Select to 31
//     c. G0_Gate_Polarity to 1
//     d. G0_Gating_Mode to kG0_Gating_ModeGating_Disabled
//     e. G0_Up_Down to kG0_Up_DownSoftware_Down
//  3. Set GPCT0_INITIAL_COUNT to match a value to count for the correct time
//     based upon the timebase chosen in 2a.


////////////////////////////////////////////////////////////////////////////////
//  Possible Gi_Source_Select values:
//  0: TB1(20 MHz)
//  1-10: PFI<0..9>
//  11-17: RTSI<0..6>
//  18: TB2(100 kHz)
//  19: The G_TC signal from the other general-purpose counter
//  20: The G_GATE signal from the other counter, when G0_Src_SubSelect = 0;
//      Star_Trig when G0_Src_SubSelect = 1
//  21-26: PFI<10..15>
//  27: RTSI7
//  28: TB1(20 MHz)
//  29: PXIClk10
//  30: The internal signal TB3(80 MHz), when G0_Src_SubSelect = 0;
//      The output of the Analog Trigger module when G0_Src_SubSelect = 1
//  31: Logic low
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//  Possible Gi_Gate_Select values
//  0: The output of the TimeStamping Mux.
//     This signal is selected with the G0_TimeStamp_Select bitfield
//  1-10: PFI<0..9>
//  11-17: RTSI<0..6>
//  18: The internal analog input signal AI_START2
//  19: Star_Trig
//  20: The G_OUT signal from the other general-purpose counter
//  21-26: PFI<10..15>
//  27: RTSI7
//  28: The internal analog input signal AI_START1
//  29: The G_SOURCE signal from the other general-purpose counter
//  30: The output of the Analog Trigger Module
//  31: Logic low
////////////////////////////////////////////////////////////////////////////////


#include <stdio.h>

#ifndef ___tMSeries_h___
 #include "tMSeries.h"
#endif

#ifndef ___tTIO_h___
 #include "tTIO.h"
#endif

#ifndef ___common_h___
 #include "common.h"
#endif

#define GPCT0_INITIAL_COUNT 0x0

void resetGPCT0(tTIO *board);
void armGPCT0(tTIO *board);
void disarmGPCT0(tTIO *board);
void PeriodMeasurementConfigure (tTIO *board, tMSeries *boardM);
u32 readSaveRegGPCT0(tTIO *board);

void test(iBus *bus)
{

    // create register map

    tAddressSpace  bar1;
    tMSeries *boardM;
    tTIO *board;

    bar1 = bus->createAddressSpace(kPCI_BAR1);
    boardM = new tMSeries(bar1);
    board = new tTIO(bar1);
    //TIO chip on MSeries boards is at offset 0x100
	board->setAddressOffset(0x100);

    configureTimebase(boardM);

    resetGPCT0(board);

    PeriodMeasurementConfigure (board, boardM);

	//Set PFI pins to be input
	boardM->IO_Bidirection_Pin.writeRegister (0x00);

    armGPCT0(board);
    
    while ( board->G01_Status.readG0_Armed_St() )
    {
        // wait for complete measurement (counter disarm)
    }
    
    u32 value = 0; 
    
    value = board->G0_Save.readG0_Save_Value();
    
    if ( board->G0_Status_1.readG0_TC_St() )
    {
        printf ("rollover error!!!\n");
    }
    
    disarmGPCT0(board);

    float period; 
    
    period = value / 20000.0;   // period (ms) = (count / timebase ) * 1000
    printf("period : %f ms (count : %d)\n", period, value);

    //cleanup
    delete boardM;
    delete board;
    bus->destroyAddressSpace(bar1);
}

void resetGPCT0(tTIO *board)
{
    // Reset GPCT0

    board->G01_Joint_Reset.writeG0_Reset(1);
    board->G0_Mode.writeRegister(0);
    board->G0_Command.writeRegister(0);
    board->G0_Input_Select.writeRegister(0);
    board->G0_AutoIncrement.writeRegister(0);
    
    board->Interrupt_G0_Enable.setG0_TC_Interrupt_Enable(0);
    board->Interrupt_G0_Enable.setG0_Gate_Interrupt_Enable(0);
    board->Interrupt_G0_Enable.flush();
    
    board->G0_Command.writeG0_Synchronized_Gate(tTIO::tG0_Command::kG0_Synchronized_GateEnabled);
    board->Interrupt_G0_Ack.setG0_Gate_Error_Confirm(1);
    board->Interrupt_G0_Ack.setG0_TC_Error_Confirm(1);
    board->Interrupt_G0_Ack.setG0_TC_Interrupt_Ack(1);
    board->Interrupt_G0_Ack.setG0_Gate_Interrupt_Ack(1);
    board->Interrupt_G0_Ack.flush();
    
    board->G0_AutoIncrement.write(0);
    
    board->G0_MSeries_Counting_Mode.setG0_MSeries_Alternate_Synchronization (0); // 1 if using 80 MHz 
    board->G0_MSeries_Counting_Mode.setG0_MSeries_Index_Enable (0);
    board->G0_MSeries_Counting_Mode.setG0_MSeries_Prescale (0);
    board->G0_MSeries_Counting_Mode.setG0_MSeries_Encoder_Counting_Mode (0);
    board->G0_MSeries_Counting_Mode.flush();
    
    board->G0_Second_Gate.setG0_Second_Gate_Select(0);
    board->G0_Second_Gate.setG0_Second_Gate_Polarity(0);
    board->G0_Second_Gate.flush();
}

void armGPCT0(tTIO *board)
{
    //Arm CPCT0

    board->G0_Command.writeG0_Arm(1);
}

void disarmGPCT0(tTIO *board)
{
    //Disarm CPCT0

    board->G0_Command.writeG0_Disarm(1);
}

u32 readSaveRegGPCT0(tTIO *board)
{
    //Read save register twice and compare.
    //If not equal, then read third time and return.

    u32 value1, value2;

    board->G0_Command.writeG0_Save_Trace(0);
    board->G0_Command.writeG0_Save_Trace(1);

    value1 = board->G0_Save.readG0_Save_Value();
    value2 = board->G0_Save.readG0_Save_Value();

    if(value1 != value2)
    {
        value1 = board->G0_Save.readG0_Save_Value();
    }

    return value1;
}

void PeriodMeasurementConfigure (tTIO *board,  tMSeries *boardM)
{

    board->G0_Mode.writeG0_Load_Source_Select(tTIO::tG0_Mode::kG0_Load_Source_SelectLoad_A);
    
    board->G0_Load_A.writeRegister(0);
    
    board->G0_Command.writeG0_Load(1);

    board->G0_Input_Select.setG0_Source_Select (28);  // 20 MHz
    board->G0_Input_Select.setG0_Source_Polarity(0); // rising=0
    
    board->G0_Input_Select.setG0_OR_Gate(0);    
    board->G0_Input_Select.setG0_Output_Polarity(0); //active low=0    
    board->G0_Input_Select.setG0_Gate_Select_Load_Source(0);

    board->G0_Input_Select.setG0_Gate_Select(10); // PFI9
    
    board->G0_Input_Select.flush ();    
    
    board->G0_Mode.setG0_Gate_Polarity(0); //invert=1
    
    board->G0_Mode.setG0_Output_Mode(tTIO::tG0_Mode::kG0_Output_ModePulse);
    board->G0_Mode.setG0_Loading_On_TC(tTIO::tG0_Mode::kG0_Loading_On_TCRollover_On_TC);
    board->G0_Mode.setG0_Gate_On_Both_Edges (tTIO::tG0_Mode::kG0_Gate_On_Both_EdgesBoth_Edges_Disabled);
    board->G0_Mode.setG0_Stop_Mode(tTIO::tG0_Mode::kG0_Stop_ModeStop_On_Gate);
    board->G0_Mode.setG0_Counting_Once(tTIO::tG0_Mode::kG0_Counting_OnceDisarm_On_Gate);
    
    // single-period measurement
    board->G0_Mode.setG0_Reload_Source_Switching(tTIO::tG0_Mode::kG0_Reload_Source_SwitchingAlternate);
    board->G0_Mode.setG0_Loading_On_Gate(tTIO::tG0_Mode::kG0_Loading_On_GateNo_Reload);
    board->G0_Mode.setG0_Gating_Mode (tTIO::tG0_Mode::kG0_Gating_ModeEdge_Gating_Active_High);
    board->G0_Mode.setG0_Trigger_Mode_For_Edge_Gate(tTIO::tG0_Mode::kG0_Trigger_Mode_For_Edge_GateFirst_Starts_Next_Stops);

    board->G0_Mode.flush();    
    
    board->G0_Command.setG0_Up_Down(tTIO::tG0_Command::kG0_Up_DownSoftware_Up); //kG0_Up_DownSoftware_Down
    board->G0_Command.setG0_Bank_Switch_Enable(tTIO::tG0_Command::kG0_Bank_Switch_EnableBank_X);
    board->G0_Command.setG0_Bank_Switch_Mode(tTIO::tG0_Command::kG0_Bank_Switch_ModeGate);
    board->G0_Command.flush();

    board->Interrupt_G0_Enable.setG0_TC_Interrupt_Enable(0);
    board->Interrupt_G0_Enable.setG0_Gate_Interrupt_Enable(0);
    board->Interrupt_G0_Enable.flush();
    
    boardM->G0_DMA_Config.writeG0_DMA_Reset(1);
    
    boardM->G0_DMA_Config.setG0_DMA_Enable(0);
    boardM->G0_DMA_Config.setG0_DMA_Int_Enable(0);
    boardM->G0_DMA_Config.setG0_DMA_Write(0);
    boardM->G0_DMA_Config.flush();
    
}

