// 
//  ao.h 
//
//  $DateTime: 2010/08/12 14:41:45 $
//
#ifndef ___ao_h___
#define ___ao_h___

#ifndef ___t6723_h___
	#include "t6723.h"
#endif

#ifndef ___tSTC_h___
	#include "tSTC.h"
#endif

void aoReset(tSTC *theSTC);  
void aoClock(tSTC *theSTC);
void aoPersonalize(tSTC *theSTC);
void aoStartTrigger(tSTC *theSTC, 
                tSTC::tAO_Trigger_Select::tAO_START1_Select source,
                tSTC::tAO_Trigger_Select::tAO_START1_Polarity polarity);
void aoCount(tSTC *theSTC, u32 numberOfSamples, u32 numberOfBuffers, tBoolean continuous);
void aoUpdate(tSTC *theSTC, u32 updateInterval);
void aoChannelSelect(tSTC *theSTC);
void aoConfigureDAC(tSTC *theSTC);
void aoFifoMode(tSTC* theSTC, tBoolean fifoRetransmit);
void aoStop(tSTC *theSTC, tBoolean isContinuous);
void aoArm(tSTC *theSTC);
void aoStart(tSTC *theSTC);
int aoComplete(tSTC *theSTC, tBoolean isContinuous);
void aoInterrupt(tSTC *theSTC);
void aoDisarm(tSTC *theSTC);
void aoClearFifo(tSTC *theSTC);

#endif // ___ao_h___
