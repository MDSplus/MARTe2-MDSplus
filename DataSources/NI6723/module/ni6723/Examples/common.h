//
//  common.h
//
//  $DateTime: 2010/08/12 14:41:45 $
//
#ifndef ___common_h___
#define ___common_h___

#ifndef ___t6723_h___
	#include "t6723.h"
#endif

#ifndef ___tSTC_h___
	#include "tSTC.h"
#endif

void spin(void);
u8 readFromEEPROM(t6723 *board, u16 address);
double u32ToDouble(u32 value);
bool FPGA_Check(t6723 *board, u32 expectedCheck);

#endif // ___common_h___

