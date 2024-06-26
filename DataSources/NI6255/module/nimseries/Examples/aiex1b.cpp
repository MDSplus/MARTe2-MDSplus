//
//  aiex1.cpp --
//
//      On-demand acquisition
//
//  $DateTime: 2006/10/24 23:40:45 $
//
#include "tMSeries.h"
#include "ai.h"
#include "ao.h"

#include "common.h"
#include "scale.h"

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <unistd.h>
#include <sched.h>
#include <errno.h>

       #include <sys/stat.h>
       #include <fcntl.h>
#include <sys/mman.h>

#define rdtscll(val) __asm__ __volatile__ ("rdtsc " : "=A" (val))

void test(iBus *bus)
{
    u32 numberOfSamples = 0xFFFFFF;    
    u32 numberOfChannels = 4;  

// setting scheduling option for multiprocessors systems
    cpu_set_t cpu_mask;
    CPU_ZERO(&cpu_mask);
    CPU_SET(1, &cpu_mask);
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
    mlockall(MCL_FUTURE);
    //  read eeprom for calibration information
    
    const u32 kEepromSize = 1024;
    u8 eepromMemory[kEepromSize];
    eepromReadMSeries (bus, eepromMemory, kEepromSize);   

    // create register map
    
   tAddressSpace bar1;
   tMSeries *board;

   bar1 = bus->createAddressSpace(kPCI_BAR1);
   board = new tMSeries(bar1);
    
    // ---- AI/AO Reset ----
    //

    configureTimebase (board);
    pllReset (board);
    analogTriggerReset (board);
    
    aiReset (board);
    // check ai.h for aiPersonalize allowed values
    aiPersonalize (board, tMSeries::tAI_Output_Control::kAI_CONVERT_Output_SelectActive_Low);
    aiClearFifo (board);

    aoReset (board);
    aoPersonalize (board);
    aoResetWaveformChannels (board);
    aoClearFifo (board);

    // unground AO reference
    board->AO_Calibration.writeAO_RefGround (kFalse);
    
    // ADC reset only applies to 625x boards
    adcReset(board);

    // ---- End of AI/AO Reset ----
    
    // ---- Start A0 task ---
    u32 channel = 0; 
    tScalingCoefficients AOscale;
    aoGetScalingCoefficients (eepromMemory, 0, 0, channel, &AOscale);  
    
    aoConfigureDAC (board, 
                     channel, 
                     0xF, 
                     tMSeries::tAO_Config_Bank::kAO_DAC_PolarityBipolar,
                     tMSeries::tAO_Config_Bank::kAO_Update_ModeImmediate);

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

int irqfd;

  irqfd = open("/proc/nirlpk/37", 0);
printf("irqfd = %d\n", irqfd);

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

	board->Joint_Reset.setAI_Configuration_Start (1);
        board->Joint_Reset.flush();

board->Interrupt_Control.writeRegister(0);
board->Interrupt_Control.flush();
board->Interrupt_Control.setInterrupt_Group_A_Enable(1);
board->Interrupt_Control.flush();

board->Interrupt_A_Enable.writeAI_START_Interrupt_Enable(1);
//board->Interrupt_A_Enable.writeAI_STOP_Interrupt_Enable(1);
board->Interrupt_A_Enable.flush();

    board->Joint_Reset.setAI_Configuration_End (1);
    board->Joint_Reset.flush();
    
/*
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
		board->AI_Status_1.readAI_FIFO_Request_St() ? "1" : "0");
*/
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
    int nints;   
    while (n < numberOfSamples)
    {
        // trigger a scan
        //aiStartOnDemand (board);    
        joint_stat = 0;
	joint_stat_2 = 0;
//        while ( ! (!board->Joint_Status_2.readAI_Scan_In_Progress_St() && board->AI_Status_1.readAI_FIFO_Empty_St()) )

// interrupts...
	while(read(irqfd, &nints, sizeof(nints)) != sizeof nints) {
	}
board->Interrupt_A_Ack.setAI_START_Interrupt_Ack(1);
//board->Interrupt_A_Ack.setAI_STOP_Interrupt_Ack(1);
board->Interrupt_A_Ack.flush();

// board->Interrupt_A_Enable.writeAI_START_Interrupt_Enable(1);
//board->Interrupt_A_Enable.writeAI_STOP_Interrupt_Enable(1);
// board->Interrupt_A_Enable.flush();

/*        while ( !board->Joint_Status_2.readAI_Scan_In_Progress_St() &&
	     board->AI_Status_1.readAI_FIFO_Empty_St())
        {
            // waiting for scan to complete
	    joint_stat++;
        }
*/      
/*	while ( board->Joint_Status_2.readAI_Scan_In_Progress_St() &&
	     board->AI_Status_1.readAI_FIFO_Empty_St())
        {
            // waiting for scan to complete
	    joint_stat_2++;
        }
*/
	while ( board->AI_Status_1.readAI_FIFO_Empty_St() )
	{
	    // wait for something in the FIFO
	    joint_stat_2++;
	}
         
	do {

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
  
	} while (!board->AI_Status_1.readAI_FIFO_Empty_St() );

/*	for (m=0; m<numberOfChannels; m++) {
            aiPolynomialScaler (&(value[m]), &scaled, &scale);
            printf ("%e, %d\n", scaled, astat[m]);
	}
*/

    //aoLinearScaler (&value, &voltage, &AOscale);
    
    //printf ("voltage: %f (%d)\n", voltage, value);
    //board->DAC_Direct_Data[channel].writeRegister (value);
    board->DAC_Direct_Data[channel].writeRegister (value[0]);

if (stop > 30000) {
	printf("time %lld ", stop);
	printf("stat: %d joint_stat: %d joint_stat_2: %d\n", stat, joint_stat, joint_stat_2);
}

/*	printf("FIFO_Full:%s_Half_Full:%s_Empty:%s Overrun:%sOverflow:%s "
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
*/
/*	u16 ERRstatus = board->AI_Status_1.readRegister();
	if (ERRstatus != 0x10b0)
		printf("%hx\n", ERRstatus);
*/
        n++;
    }
    free(value);
    // --- Stop ----
    
    aiDisarm (board);
    
close(irqfd);
munlockall();

    // cleanup
    delete board;
    bus->destroyAddressSpace(bar1);

    return; 
} 
