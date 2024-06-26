//
//  aiex2.cpp --
//
//      Hardware timed acquisition
//
//  $DateTime: 2006/10/24 23:40:45 $
//
#include "tMSeries.h"
#include "ai.h"
#include "common.h"
#include "scale.h"

#include <stdio.h>

void test(iBus *bus)
{
    u32 numberOfSamples     = 100;    
    u32 numberOfChannels    = 1;  
    u32 samplePeriodDivisor = 2000; // timebase/sample rate => 20 MHz / 10 kHz
    tBoolean continuous     = kFalse;
    
    //  read eeprom for calibration information
    
    const u32 kEepromSize = 1024;
    u8 eepromMemory[kEepromSize];
    eepromReadMSeries (bus, eepromMemory, kEepromSize);   

    // create register map    
    
    tAddressSpace  bar1;
    tMSeries *board;

    bar1 = bus->createAddressSpace(kPCI_BAR1);
    board = new tMSeries(bar1);
    
    // ---- AI Reset ----
    //

    configureTimebase (board);
    pllReset (board);
    analogTriggerReset (board);
    
    aiReset (board);
    // check ai.h for aiPersonalize allowed values
    aiPersonalize (board, tMSeries::tAI_Output_Control::kAI_CONVERT_Output_SelectActive_Low);
    aiClearFifo (board);
    
    // ADC reset only applies to 625x boards
    adcReset(board);

    // ---- End of AI Reset ----
    
    // ---- Start AI task ----
    
    aiDisarm (board);
    
    aiClearConfigurationMemory (board);
    
    for (u32 i = 0; i < numberOfChannels; i++)
    {        
        aiConfigureChannel (board, 
                            i,  // channel number
                            1,  // gain -- check ai.h for allowed values
                            tMSeries::tAI_Config_FIFO_Data::kAI_Config_PolarityBipolar,
                            tMSeries::tAI_Config_FIFO_Data::kAI_Config_Channel_TypeDifferential, 
                            (i == numberOfChannels-1)?kTrue:kFalse); // last channel?
    }

    aiSetFifoRequestMode (board);    
    
    aiEnvironmentalize (board);
    
    aiHardwareGating (board);
    
    aiTrigger (board,
               tMSeries::tAI_Trigger_Select::kAI_START1_SelectPulse,
               tMSeries::tAI_Trigger_Select::kAI_START1_PolarityRising_Edge,
               tMSeries::tAI_Trigger_Select::kAI_START2_SelectPulse,
               tMSeries::tAI_Trigger_Select::kAI_START2_PolarityRising_Edge);
    
    aiSampleStop (board, 
                  (numberOfChannels > 1)?kTrue:kFalse); // multi channel?
    
    aiNumberOfSamples (board,   
                       numberOfSamples, // posttrigger samples
                       0,               // pretrigger samples
                       continuous);     // continuous?
    
    aiSampleStart (board, 
                   samplePeriodDivisor, 
                   3, 
                   tMSeries::tAI_START_STOP_Select::kAI_START_SelectSI_TC,
                   tMSeries::tAI_START_STOP_Select::kAI_START_PolarityRising_Edge);
    
    aiConvert (board, 
               280,     // convert period divisor
               3,       // convert delay divisor 
               kFalse); // external sample clock?
    
    aiClearFifo (board);
    
    tScalingCoefficients scale;
    
    // check scale.h for scaling index values
    aiGetScalingCoefficients (eepromMemory, 0, 0, 0, &scale);  
    
    aiArm (board, kTrue);
    aiStart (board);
           
  
    // ---- Read FIFO ----
    
    i32 value;
    f32 scaled;
    
    u32 n = 0;  

    while (n < numberOfChannels*numberOfSamples)
    { 
        if(!board->AI_Status_1.readAI_FIFO_Empty_St())
        {
            value = board->AI_FIFO_Data.readRegister ();
            aiPolynomialScaler (&value, &scaled, &scale);
            printf ("%e,\n", scaled);
            n++;
        }
    }
    
    // ---- Stop ----
    
    aiDisarm (board);
    
    // cleanup
    delete board;
    bus->destroyAddressSpace(bar1);

    return; 
} 
