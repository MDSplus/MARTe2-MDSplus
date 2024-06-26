//
//  aoex2.cpp --
//
//      multi-channel On-demand generation
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

    f32 voltage0 = 3.5;
    f32 voltage1 = -1.75;
    
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
    
    // ---- Start A0 task ---
    
    tScalingCoefficients scale0;
    tScalingCoefficients scale1;
    aoGetScalingCoefficients (eepromMemory, 0, 0, 0, &scale0);  
    aoGetScalingCoefficients (eepromMemory, 0, 0, 1, &scale1);  
    
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
    
    // ---- Write to DACs ----
    
    i32 value; 
    
    aoLinearScaler (&value, &voltage0, &scale0);
    printf ("voltage 0: %f (%d)\n", voltage0, value);
    board->DAC_Direct_Data[0].writeRegister (value);
    
    aoLinearScaler (&value, &voltage1, &scale1);
    printf ("voltage 1: %f (%d)\n", voltage1, value);
    board->DAC_Direct_Data[1].writeRegister (value);
    
    // ---- Stop ----

    // cleanup
    delete board;
    bus->destroyAddressSpace(bar1);

    return; 
} 
