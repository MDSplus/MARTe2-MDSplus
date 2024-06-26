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
#include <stdlib.h>

#include <sys/types.h>
#include <unistd.h>
#include <sched.h>
#include <errno.h>

#define rdtscll(val) __asm__ __volatile__ ("rdtsc " : "=A" (val))

void test(iBus *bus)
{
    u32 numberOfSamples = 10;    
    u32 numberOfChannels = 4;  

// setting scheduling option for multiprocessors systems
    cpu_set_t cpu_mask;
    CPU_ZERO(&cpu_mask);
    CPU_SET(0, &cpu_mask);
    if (sched_setaffinity((pid_t) 0, (unsigned int) sizeof(cpu_mask), &cpu_mask) <0) {
	perror("sched_setaffinity ");
	return;
    }
    if (sched_getaffinity((pid_t) 0, (unsigned int) sizeof(cpu_mask), &cpu_mask) <0) {
	perror("sched_getaffinity ");
	return;
    }
    printf("cpu_mask ");
    int i;
    for (i=0; i<sizeof(cpu_mask); i++)
        printf("%d", CPU_ISSET(i, &cpu_mask));
    printf("\n");

    struct sched_param sparam;
    sparam.sched_priority = sched_get_priority_max(SCHED_FIFO);
    if (sched_setscheduler((pid_t) 0, SCHED_FIFO, &sparam) <0) {
	perror("sched_setscheduler ");
	return;
    }
    printf("SCHED_FIFO set_priority_max\n");
    
    //  read eeprom for calibration information
    
    const u32 kEepromSize = 1024;
    u8 eepromMemory[kEepromSize];
    eepromReadMSeries (bus, eepromMemory, kEepromSize);   

    // create register map
    
   tAddressSpace bar1;
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
                            tMSeries::tAI_Config_FIFO_Data::kAI_Config_Channel_TypeNRSE,
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
    
    exportSignal(board, kAIStartTrigger,kPFI0, kFalse);
    exportSignal(board, kAIConvertClock,kPFI1, kTrue);
    aiSampleStart (board, 
                   0,
                   0,
                   tMSeries::tAI_START_STOP_Select::kAI_START_SelectPFI0,
                   tMSeries::tAI_START_STOP_Select::kAI_START_PolarityRising_Edge);
    
    aiConvert (board, 
               28,     // convert period divisor
               28,     // convert delay divisor 
               kTrue); // external sample clock?
    
    aiClearFifo (board);
    
    
    tScalingCoefficients scale;
    
    // check scale.h for scaling index values
    aiGetScalingCoefficients (eepromMemory, 0, 0, 0, &scale);

    int a;  
    scanf("%d", &a);
	printf("FIFO_Full:%s_Half_Full:%s_Empty:%s Overrun:%sOverflow:%s "
               "SC_TC_Error:%sSTART2:%sSTART1:%sSC_TC:%sSTART:%sSTOP:%s "
               "G0_TC:%s_GateInterrupt:%s FIFO_Request:%s\n",
		board->AI_Status_1.readAI_FIFO_Full_St() ? "1" : "0",
		board->AI_Status_1.readAI_FIFO_Half_Full_St() ? "1" : "0",
		board->AI_Status_1.readAI_FIFO_Empty_St() ? "1" : "0",
		board->AI_Status_1.readAI_Overrun_St() ? "1" : "0",
		board->AI_Status_1.readAI_Overflow_St() ? "1" : "0",
		board->AI_Status_1.readAI_SC_TC_Error_St() ? "1" : "0",
		board->AI_Status_1.readAI_START2_St() ? "1" : "0",
		board->AI_Status_1.readAI_START1_St() ? "1" : "0",
		board->AI_Status_1.readAI_SC_TC_St() ? "1" : "0",
		board->AI_Status_1.readAI_START_St() ? "1" : "0",
		board->AI_Status_1.readAI_STOP_St() ? "1" : "0",
		board->AI_Status_1.readG0_TC_St() ? "1" : "0",
		board->AI_Status_1.readG0_Gate_Interrupt_St() ? "1" : "0",
		board->AI_Status_1.readAI_FIFO_Request_St() ? "1" : "0" );


    aiArm (board, kFalse); 
    aiStart (board);
           
  
    // ---- Read FIFO ----
    
    //i32 value;
    i32 *value;
    value = (i32*) malloc(sizeof(i32)*numberOfChannels);
    f32 scaled;
    u32 n = 0; // number of samples counter 
    u32 m = 0; // number of channels counter
    u32 stat = 0;
    u32 *astat;
    astat = (u32*) malloc(sizeof(u32)*numberOfChannels);
    u32 joint_stat = 0;
    u32 joint_stat_2 = 0;

// timing statistics
    unsigned long start;
    unsigned long stop;
// timing statistics
   
    while (n < numberOfSamples)
    {
        // trigger a scan
        //aiStartOnDemand (board);    
        joint_stat = 0;
	joint_stat_2 = 0;
//        while ( ! (!board->Joint_Status_2.readAI_Scan_In_Progress_St() && board->AI_Status_1.readAI_FIFO_Empty_St()) )
        while ( !board->Joint_Status_2.readAI_Scan_In_Progress_St() )
        {
            // waiting for scan to complete
	    joint_stat++;
        }
	while ( board->Joint_Status_2.readAI_Scan_In_Progress_St() )
        {
            // waiting for scan to complete
	    joint_stat_2++;
        }
            
        m = 0;
        stat = 0;
rdtscll(start);
        while ( m < numberOfChannels )
        {
            //if(!board->AI_Status_1.readAI_FIFO_Empty_St()) {
                value[m] = board->AI_FIFO_Data.readRegister ();
//                astat[m] = stat;
                m++;
	    //}
//            stat++;
        }
rdtscll(stop);
stop -= start;

/*	for (m=0; m<numberOfChannels; m++) {
            aiPolynomialScaler (&(value[m]), &scaled, &scale);
            printf ("%e, %d\n", scaled, astat[m]);
	}
*/        
	printf("time %lld stat: %d joint_stat: %d joint_stat_2: %d\n", stop, stat, joint_stat, joint_stat_2);
	printf("FIFO_Full:%s_Half_Full:%s_Empty:%s Overrun:%sOverflow:%s "
               "SC_TC_Error:%sSTART2:%sSTART1:%sSC_TC:%sSTART:%sSTOP:%s "
               "G0_TC:%s_GateInterrupt:%s FIFO_Request:%s\n",
		board->AI_Status_1.readAI_FIFO_Full_St() ? "1" : "0",
		board->AI_Status_1.readAI_FIFO_Half_Full_St() ? "1" : "0",
		board->AI_Status_1.readAI_FIFO_Empty_St() ? "1" : "0",
		board->AI_Status_1.readAI_Overrun_St() ? "1" : "0",
		board->AI_Status_1.readAI_Overflow_St() ? "1" : "0",
		board->AI_Status_1.readAI_SC_TC_Error_St() ? "1" : "0",
		board->AI_Status_1.readAI_START2_St() ? "1" : "0",
		board->AI_Status_1.readAI_START1_St() ? "1" : "0",
		board->AI_Status_1.readAI_SC_TC_St() ? "1" : "0",
		board->AI_Status_1.readAI_START_St() ? "1" : "0",
		board->AI_Status_1.readAI_STOP_St() ? "1" : "0",
		board->AI_Status_1.readG0_TC_St() ? "1" : "0",
		board->AI_Status_1.readG0_Gate_Interrupt_St() ? "1" : "0",
		board->AI_Status_1.readAI_FIFO_Request_St() ? "1" : "0" );

        n++;
    }
    free(value);
    // --- Stop ----
    
    aiDisarm (board);
    
    // cleanup
    delete board;
    bus->destroyAddressSpace(bar1);

    return; 
} 
