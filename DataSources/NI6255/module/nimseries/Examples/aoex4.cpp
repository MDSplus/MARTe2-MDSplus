//
//  aoex4.cpp --
//
//      multi-channel hardware timed generation
//
//  $DateTime: 2006/10/24 23:40:45 $
//
#include <stdio.h>

#ifndef ___tMSeries_h___
 #include "tMSeries.h"
#endif

#ifndef ___ao_h___
 #include "ao.h"
#endif 

#ifndef ___common_h___
 #include "common.h"
#endif

#ifndef ___scale_h___
 #include "scale.h"
#endif


void test(iBus *bus)
{
    tBoolean continuous = kFalse; 
    tBoolean fifoRetransmit = kTrue; 
    u32 updatePeriodDivisor = 200000; 
    u32 numberOfSamples = 10; 
    
    f32 voltage0[10] = {-5, -4, -3, -2, -1, 0, 1, 2, 3, 4};
    f32 voltage1[10] = {4, 3, 2, 1, 0, -1, -2, -3, -4, -5};
    
    //  read eeprom for calibration information
    
    const u32 kEepromSize = 1024;
    u8 eepromMemory[kEepromSize];
    eepromReadMSeries (bus, eepromMemory, kEepromSize);   

    // create register map
    
	tAddressSpace  bar1;
	tMSeries *board;

	bar1 = bus->createAddressSpace(kPCI_BAR1);
	board = new tMSeries(bar1);
    
    // ---- AO Reset ----
    // 
    
    configureTimebase (board);
    pllReset (board);
    analogTriggerReset (board);
    
    aoReset (board);
    aoPersonalize (board);
    aoResetWaveformChannels (board);
    aoClearFifo (board);
    
    // unground AO reference
    board->AO_Calibration.writeAO_RefGround (kFalse);
        
    // ---- End of AO Reset ----
    
    // ---- Write to FIFO ----
    
    tScalingCoefficients scale0;
    tScalingCoefficients scale1;

    aoGetScalingCoefficients (eepromMemory, 0, 0, 0, &scale0);    
    aoGetScalingCoefficients (eepromMemory, 0, 0, 1, &scale1);    
    
    i32 value; 
    
    for (u32  i=0; i<numberOfSamples; i++)
    {
        aoLinearScaler (&value, &voltage0[i], &scale0);
        board->AO_FIFO_Data.writeRegister (value); 
        aoLinearScaler (&value, &voltage1[i], &scale1);
        board->AO_FIFO_Data.writeRegister (value); 
    }

    // ---- Start A0 Task ---
    
    aoConfigureDAC (board, 
                     0, 
                     0,  //wavefrom order
                     tMSeries::tAO_Config_Bank::kAO_DAC_PolarityBipolar,
                     tMSeries::tAO_Config_Bank::kAO_Update_ModeTimed);
    aoConfigureDAC (board, 
                     1, 
                     1,  //wavefrom order
                     tMSeries::tAO_Config_Bank::kAO_DAC_PolarityBipolar,
                     tMSeries::tAO_Config_Bank::kAO_Update_ModeTimed);
    
    aoChannelSelect (board, 2);
    aoTrigger (board, 
               tMSeries::tAO_Trigger_Select::kAO_START1_SelectPulse,
               tMSeries::tAO_Trigger_Select::kAO_START1_PolarityRising_Edge);
    aoCount (board, numberOfSamples, 1, continuous);
    aoUpdate (board, 
              tMSeries::tAO_Mode_1::kAO_UPDATE_Source_SelectUI_TC, 
              tMSeries::tAO_Mode_1::kAO_UPDATE_Source_PolarityRising_Edge, 
              updatePeriodDivisor);
    aoFifoMode (board, fifoRetransmit);
    aoStop (board);
    aoArm (board);
    aoStart (board);
    

    // ---- Wait for task to complete ----
    
    tBoolean stop = kFalse; 
    
    while (!stop)
    {
        if (board->AO_Status_1.readAO_Overrun_St())
        {
            printf ("FIFO overrun error\n");
            stop = kTrue; 
        }
        
        if (!board->AO_Status_2.readAO_BC_Armed_St())
        {
            printf ("AO task stopped normally\n");
            stop = kTrue; 
        }  
    }

    // ---- Stop ---- 
    
    aoDisarm (board);    
    
    // reset the dacs
    aoConfigureDAC (board, 
                     0, 
                     0xF, 
                     tMSeries::tAO_Config_Bank::kAO_DAC_PolarityBipolar,
                     tMSeries::tAO_Config_Bank::kAO_Update_ModeImmediate);
    aoConfigureDAC (board, 
                     1, 
                     0xF, 
                     tMSeries::tAO_Config_Bank::kAO_DAC_PolarityBipolar,
                     tMSeries::tAO_Config_Bank::kAO_Update_ModeImmediate);
    
    // cleanup
    delete board;
    bus->destroyAddressSpace(bar1);

    return; 
} 
