//
//  aiex1.cpp --
//
//      On-demand acquisition
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
    u32 numberOfSamples = 10;    
    u32 numberOfChannels = 2;  
    
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
    
    // fill configuration FIFO 
    // Note: It is not necessary for the channel numbers to be ordered
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
                       1,      // posttrigger samples
                       0,      // pretrigger samples
                       kTrue); // continuous?
    
    aiSampleStart (board, 
                   0,
                   0,
                   tMSeries::tAI_START_STOP_Select::kAI_START_SelectPulse,
                   tMSeries::tAI_START_STOP_Select::kAI_START_PolarityRising_Edge);
    
    aiConvert (board, 
               280,     // convert period divisor
               280,     // convert delay divisor 
               kFalse); // external sample clock?
    
    aiClearFifo (board);
    
    
    tScalingCoefficients scale;
    
    // check scale.h for scaling index values
    aiGetScalingCoefficients (eepromMemory, 0, 0, 0, &scale);  
    
    aiArm (board, kFalse); 
    aiStart (board);
           
  
    // ---- Read FIFO ----
    
    i32 value;
    f32 scaled;
    u32 n = 0; // number of samples counter 
    u32 m = 0; // number of channels counter
   
    while (n < numberOfSamples)
    {
        // trigger a scan
        aiStartOnDemand (board);    
        
        while ( board->Joint_Status_2.readAI_Scan_In_Progress_St())
        {
            // waiting for scan to complete
        }
            
        m = 0;
        while ( m < numberOfChannels )
        {
            if(!board->AI_Status_1.readAI_FIFO_Empty_St())
            {
                value = board->AI_FIFO_Data.readRegister ();
                aiPolynomialScaler (&value, &scaled, &scale);
                printf ("%e,\n", scaled);
                m++;
            }
        }
        n++;
    }
    
    // --- Stop ----
    
    aiDisarm (board);
    
    // cleanup
    delete board;
    bus->destroyAddressSpace(bar1);

    return; 
} 
