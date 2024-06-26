//
//  common.h
//
//  $DateTime: 2006/10/24 23:40:45 $
//
#ifndef ___common_h___
#define ___common_h___

#ifndef ___tMSeries_h___
 #include "tMSeries.h"
#endif

void configureTimebase  (tMSeries* board);
void pllReset           (tMSeries* board);
void analogTriggerReset (tMSeries* board);

//
// Export Signal only allows direct routes
//
enum tTerminal
{   
   kRTSITerminal = 1,
   kRTSI0,   
   kRTSI1,   
   kRTSI2,   
   kRTSI3,   
   kRTSI4,   
   kRTSI5,   
   kRTSI6,   
   kRTSI7,
   
   kPFITerminal, 
   kPFI0,
   kPFI1,
   kPFI2,
   kPFI3,
   kPFI4,
   kPFI5,
   kPFI6,
   kPFI7,
   kPFI8,
   kPFI9,
   kPFI10,
   kPFI11,
   kPFI12,
   kPFI13,
   kPFI14,
   kPFI15,
};

enum tSignal
{
   kAISampleClock = 1, 
   kAIConvertClock, 
   kAIStartTrigger, 
   kAIReferenceTrigger,
   kAOSampleClock, 
   kAOStartTrigger,
};

enum
{
   kDisable = 0,
   kEnable  = 1, 
};

void exportSignal (tMSeries* board, 
                    tSignal source, 
                    tTerminal terminal,
                    tBoolean enable);


#endif // ___common_h___

