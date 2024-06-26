//
//  aiex1.cpp --
//
//  dual board data acquisition
//
//  $DateTime: 2006/10/24 23:40:45 $
//
#include "tMSeries.h"
#include "ai.h"
#include "ao.h"

#include "common.h"
#include "scale.h"

// DMA Support
#include "nimhddk_dma/tDMAChannel.h"
#include "nimhddk_dma/tMITE.h"

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <unistd.h>
#include <sched.h>
#include <errno.h>

#define rdtscll(val) __asm__ __volatile__ ("rdtsc " : "=A" (val))

extern void initMite(iBus *bus);			//Initialise Mite Chip.

void test(iBus *bus0)
{
    u32 numberOfSamples = 0x0100;    
    u32 numberOfChannels = 80;  

    const u32 userSizeInSamples = numberOfChannels; 
    const u32 userSizeInBytes   = userSizeInSamples * sizeof(i16);

    const u32 dmaSizeInSamples = 1000 * numberOfChannels; 
    const u32 dmaSizeInBytes   = dmaSizeInSamples * sizeof(i16); 


// initialization of other boards
	iBus* bus1;
    bus1 = acquireBoard("PXI49::15::INSTR");
    if(bus1 == NULL){
		printf("Could not access PCI device 1.  Exiting.\n");
		return ;
	}
	//Intitialise Mite Chip.
	initMite(bus1);



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
    
    //  read eeprom for calibration information
    
    const u32 kEepromSize = 1024;
    u8 eepromMemory0[kEepromSize];
    eepromReadMSeries (bus0, eepromMemory0, kEepromSize);   
    u8 eepromMemory1[kEepromSize];
    eepromReadMSeries (bus1, eepromMemory1, kEepromSize);   


    // create register map
    
   tAddressSpace bar1_0;
   tMSeries *board_0;
   tAddressSpace bar1_1;
   tMSeries *board_1;

   bar1_0 = bus0->createAddressSpace(kPCI_BAR1);
   board_0 = new tMSeries(bar1_0);
   bar1_1 = bus1->createAddressSpace(kPCI_BAR1);
   board_1 = new tMSeries(bar1_1);


   // DMA Object
 
   tAddressSpace bar0_0;
   tMITE *mite_0;
   tDMAChannel *dma_0;
   tAddressSpace bar0_1;
   tMITE *mite_1;
   tDMAChannel *dma_1;

   bar0_0 = bus0->createAddressSpace(kPCI_BAR0);
   mite_0 = new tMITE(bar0_0);
   mite_0->setAddressOffset(0x500);
   dma_0 = new tDMAChannel(bus0, mite_0);

   bar0_1 = bus1->createAddressSpace(kPCI_BAR0);
   mite_1 = new tMITE(bar0_1);
   mite_1->setAddressOffset(0x500);
   dma_1 = new tDMAChannel(bus1, mite_1);

    
    // ---- AI/AO Reset ----
    //

    configureTimebase (board_0);
    pllReset (board_0);
    analogTriggerReset (board_0);
    
    aiReset (board_0);
    // check ai.h for aiPersonalize allowed values
    aiPersonalize (board_0, tMSeries::tAI_Output_Control::kAI_CONVERT_Output_SelectActive_Low);
    aiClearFifo (board_0);

    aoReset (board_0);
    aoPersonalize (board_0);
    aoResetWaveformChannels (board_0);
    aoClearFifo (board_0);

    configureTimebase (board_1);
    pllReset (board_1);
    analogTriggerReset (board_1);
    
    aiReset (board_1);
    // check ai.h for aiPersonalize allowed values
    aiPersonalize (board_1, tMSeries::tAI_Output_Control::kAI_CONVERT_Output_SelectActive_Low);
    aiClearFifo (board_1);

    aoReset (board_1);
    aoPersonalize (board_1);
    aoResetWaveformChannels (board_1);
    aoClearFifo (board_1);

    // unground AO reference
    board_0->AO_Calibration.writeAO_RefGround (kFalse);
    board_1->AO_Calibration.writeAO_RefGround (kFalse);
    
    // ADC reset only applies to 625x boards
    adcReset(board_0);
    adcReset(board_1);

    // ---- End of AI/AO Reset ----
    
    // ---- Start A0 task ---
    u32 channel = 0; 
    tScalingCoefficients AOscale_0;
    tScalingCoefficients AOscale_1;
    aoGetScalingCoefficients (eepromMemory0, 0, 0, channel, &AOscale_0);  
    aoGetScalingCoefficients (eepromMemory1, 0, 0, channel, &AOscale_1);  
    
    aoConfigureDAC (board_0, 
                     channel, 
                     0xF, 
                     tMSeries::tAO_Config_Bank::kAO_DAC_PolarityBipolar,
                     tMSeries::tAO_Config_Bank::kAO_Update_ModeImmediate);
    aoConfigureDAC (board_1, 
                     channel, 
                     0xF, 
                     tMSeries::tAO_Config_Bank::kAO_DAC_PolarityBipolar,
                     tMSeries::tAO_Config_Bank::kAO_Update_ModeImmediate);

    // ---- Start AI task ----
    
    aiDisarm (board_0);
    aiClearConfigurationMemory (board_0);

    aiDisarm (board_1);
    aiClearConfigurationMemory (board_1);

    
    // fill configuration FIFO 
    // Note: It is not necessary for the channel numbers to be ordered
    for (u32 i = 0; i < numberOfChannels; i++)
    {        
        aiConfigureChannel (board_0, 
                            i,  // channel number
                            1,  // gain -- check ai.h for allowed values
                            tMSeries::tAI_Config_FIFO_Data::kAI_Config_PolarityBipolar,
                            tMSeries::tAI_Config_FIFO_Data::kAI_Config_Channel_TypeNRSE,
                            (i == numberOfChannels-1)?kTrue:kFalse); // last channel?
    }
    for (u32 i = 0; i < numberOfChannels; i++)
    {        
        aiConfigureChannel (board_1, 
                            i,  // channel number
                            1,  // gain -- check ai.h for allowed values
                            tMSeries::tAI_Config_FIFO_Data::kAI_Config_PolarityBipolar,
                            tMSeries::tAI_Config_FIFO_Data::kAI_Config_Channel_TypeNRSE,
                            (i == numberOfChannels-1)?kTrue:kFalse); // last channel?
    }

    aiSetFifoRequestMode (board_0);    
    aiSetFifoRequestMode (board_1);    
    
    aiEnvironmentalize (board_0);
    aiEnvironmentalize (board_1);
    
    aiHardwareGating (board_0);
    aiHardwareGating (board_1);
    
    aiTrigger (board_0,
               tMSeries::tAI_Trigger_Select::kAI_START1_SelectPulse,
               tMSeries::tAI_Trigger_Select::kAI_START1_PolarityRising_Edge,
               tMSeries::tAI_Trigger_Select::kAI_START2_SelectPulse,
               tMSeries::tAI_Trigger_Select::kAI_START2_PolarityRising_Edge);
    aiTrigger (board_1,
               tMSeries::tAI_Trigger_Select::kAI_START1_SelectPulse,
               tMSeries::tAI_Trigger_Select::kAI_START1_PolarityRising_Edge,
               tMSeries::tAI_Trigger_Select::kAI_START2_SelectPulse,
               tMSeries::tAI_Trigger_Select::kAI_START2_PolarityRising_Edge);

    aiSampleStop (board_0, 
                  (numberOfChannels > 1)?kTrue:kFalse); // multi channel?
    aiSampleStop (board_1, 
                  (numberOfChannels > 1)?kTrue:kFalse); // multi channel?

    aiNumberOfSamples (board_0,   
                       1,      // posttrigger samples
                       0,      // pretrigger samples
                       kTrue); // continuous?
    aiNumberOfSamples (board_1,   
                       1,      // posttrigger samples
                       0,      // pretrigger samples
                       kTrue); // continuous?
    
    exportSignal(board_0, kAIStartTrigger,kPFI0, kFalse);
    exportSignal(board_0, kAIConvertClock,kPFI1, kTrue);
    exportSignal(board_1, kAIStartTrigger,kPFI0, kFalse);
    exportSignal(board_1, kAIConvertClock,kPFI1, kTrue);

    aiSampleStart (board_0, 
                   0,
                   0,
                   tMSeries::tAI_START_STOP_Select::kAI_START_SelectPFI0,
                   tMSeries::tAI_START_STOP_Select::kAI_START_PolarityRising_Edge);
    aiSampleStart (board_1, 
                   0,
                   0,
                   tMSeries::tAI_START_STOP_Select::kAI_START_SelectPFI0,
                   tMSeries::tAI_START_STOP_Select::kAI_START_PolarityRising_Edge);
    
    aiConvert (board_0, 
               20,     // convert period divisor
               20,     // convert delay divisor 
               kTrue); // external sample clock?
    aiConvert (board_1, 
               20,     // convert period divisor
               20,     // convert delay divisor 
               kTrue); // external sample clock?
    
    aiClearFifo (board_0);
    aiClearFifo (board_1);
        
    tScalingCoefficients scale_0;
    tScalingCoefficients scale_1;
    
    // check scale.h for scaling index values
    aiGetScalingCoefficients (eepromMemory0, 0, 0, 0, &scale_0);
    aiGetScalingCoefficients (eepromMemory1, 0, 0, 0, &scale_1);

    // configure DMA on the device (board)
    board_0->AI_AO_Select.setAI_DMA_Select(1);
    board_0->AI_AO_Select.flush();
    board_1->AI_AO_Select.setAI_DMA_Select(1);
    board_1->AI_AO_Select.flush();

    //
    // Configure and start DMA Channel
    //
    //    DMA operations use bytes instead of samples.
    //    622x and 625x devices transfer 16-bits at a time (1 sample)
    //    628x devices transfer 32-bits at a time (1 sample)
    //    Start the DMA engine before arming/starting the AI engine
    //
//        
    tDMAError status = kNoError; 

    status = dma_0->config (0, tDMAChannel::kRing, tDMAChannel::kIn, dmaSizeInBytes, tDMAChannel::k16bit);
    if (status != kNoError)
    {
        printf ("Error: dma 0 configuration (%d)\n", status);
    }
    status = dma_1->config (0, tDMAChannel::kRing, tDMAChannel::kIn, dmaSizeInBytes, tDMAChannel::k16bit);
    if (status != kNoError)
    {
        printf ("Error: dma 1 configuration (%d)\n", status);
    }

    // print register status 
   
    int a;  
    scanf("%d", &a);
	printf("14:FIFO_Full:%s_Half_Full:%s_Empty:%s Overrun:%sOverflow:%s "
               "SC_TC_Error:%sSTART2:%sSTART1:%sSC_TC:%sSTART:%sSTOP:%s "
               "G0_TC:%s_GateInterrupt:%s FIFO_Request:%s\n",
		board_0->AI_Status_1.readAI_FIFO_Full_St() ? "1" : "0",
		board_0->AI_Status_1.readAI_FIFO_Half_Full_St() ? "1" : "0",
		board_0->AI_Status_1.readAI_FIFO_Empty_St() ? "1" : "0",
		board_0->AI_Status_1.readAI_Overrun_St() ? "1" : "0",
		board_0->AI_Status_1.readAI_Overflow_St() ? "1" : "0",
		board_0->AI_Status_1.readAI_SC_TC_Error_St() ? "1" : "0",
		board_0->AI_Status_1.readAI_START2_St() ? "1" : "0",
		board_0->AI_Status_1.readAI_START1_St() ? "1" : "0",
		board_0->AI_Status_1.readAI_SC_TC_St() ? "1" : "0",
		board_0->AI_Status_1.readAI_START_St() ? "1" : "0",
		board_0->AI_Status_1.readAI_STOP_St() ? "1" : "0",
		board_0->AI_Status_1.readG0_TC_St() ? "1" : "0",
		board_0->AI_Status_1.readG0_Gate_Interrupt_St() ? "1" : "0",
		board_0->AI_Status_1.readAI_FIFO_Request_St() ? "1" : "0" );
	printf("15:FIFO_Full:%s_Half_Full:%s_Empty:%s Overrun:%sOverflow:%s "
               "SC_TC_Error:%sSTART2:%sSTART1:%sSC_TC:%sSTART:%sSTOP:%s "
               "G0_TC:%s_GateInterrupt:%s FIFO_Request:%s\n",
		board_1->AI_Status_1.readAI_FIFO_Full_St() ? "1" : "0",
		board_1->AI_Status_1.readAI_FIFO_Half_Full_St() ? "1" : "0",
		board_1->AI_Status_1.readAI_FIFO_Empty_St() ? "1" : "0",
		board_1->AI_Status_1.readAI_Overrun_St() ? "1" : "0",
		board_1->AI_Status_1.readAI_Overflow_St() ? "1" : "0",
		board_1->AI_Status_1.readAI_SC_TC_Error_St() ? "1" : "0",
		board_1->AI_Status_1.readAI_START2_St() ? "1" : "0",
		board_1->AI_Status_1.readAI_START1_St() ? "1" : "0",
		board_1->AI_Status_1.readAI_SC_TC_St() ? "1" : "0",
		board_1->AI_Status_1.readAI_START_St() ? "1" : "0",
		board_1->AI_Status_1.readAI_STOP_St() ? "1" : "0",
		board_1->AI_Status_1.readG0_TC_St() ? "1" : "0",
		board_1->AI_Status_1.readG0_Gate_Interrupt_St() ? "1" : "0",
		board_1->AI_Status_1.readAI_FIFO_Request_St() ? "1" : "0" );

/*
board->Interrupt_Control.setInterrupt_Group_A_Enable(1);
board->Interrupt_Control.flush();

board->Interrupt_A_Enable.writeAI_START_Interrupt_Enable(1);
board->Interrupt_A_Enable.writeAI_STOP_Interrupt_Enable(1);
board->Interrupt_A_Enable.flush();

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

    status = dma_0->start();
    if ( kNoError != status )
    {
        printf ("Error: dma 0 start (%d)\n", status);
        return ;
    }
    status = dma_1->start();
    if ( kNoError != status )
    {
        printf ("Error: dma 1 start (%d)\n", status);
        return ;
    }

        //
        // No error - arm and start AI engine
        //
        aiArm (board_0, kTrue);
        aiArm (board_1, kTrue);
        aiStart (board_0);
        aiStart (board_1);

    
  
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

    u32 bytesAvailable = 0; // bytes in the DMA buffer

i32 flipflop = 0x1000;

    while (n < numberOfSamples)
    {
        // trigger a scan
        //aiStartOnDemand (board);    
/*        joint_stat = 0;
	joint_stat_2 = 0;
//        while ( ! (!board->Joint_Status_2.readAI_Scan_In_Progress_St() && board->AI_Status_1.readAI_FIFO_Empty_St()) )
        while ( !board->Joint_Status_2.readAI_Scan_In_Progress_St() &&
	     board->AI_Status_1.readAI_FIFO_Empty_St())
        {
            // waiting for scan to complete
	    joint_stat++;
        }
	while ( board->Joint_Status_2.readAI_Scan_In_Progress_St() &&
	     board->AI_Status_1.readAI_FIFO_Empty_St())
        {
            // waiting for scan to complete
	    joint_stat_2++;
        }
*/       

// ------------------------------------------------------------------------- FIRST BOARD (14)

    do 
    {
        //
        //  1. Use read to query the number of bytes in the DMA buffers
        //
        status = dma_0->read(0, NULL, &bytesAvailable);
    }  while ( (bytesAvailable < userSizeInBytes) && (kNoError == status));

//value [0] = 0xFFFFFFFF;
//printf("BeFore bytesAvailable are: %d  %x %x %x %x \n", bytesAvailable, value[0], value[1], value[2], value[3] );
rdtscll(start);
    status = dma_0->readUnsafe (userSizeInBytes, (u8 *)value);
rdtscll(stop);            
//printf("After bytesAvailable are: %d  %x %x %x %x \n", bytesAvailable, value[0], value[1], value[2], value[3] );

stop -= start;
        //
        // 3. check for errors
        //
        if (status != kNoError)
        {
            printf ("error: dma 0 read (%d)\n", status);
            break; 
        }

// ------------------------------------------------------------------------- SECOND BOARD (15)

    do 
    {
        //
        //  1. Use read to query the number of bytes in the DMA buffers
        //
        status = dma_1->read(0, NULL, &bytesAvailable);
    }  while ( (bytesAvailable < userSizeInBytes) && (kNoError == status));

//value [0] = 0xFFFFFFFF;
//printf("BeFore bytesAvailable are: %d  %x %x %x %x \n", bytesAvailable, value[0], value[1], value[2], value[3] );
rdtscll(start);
    status = dma_1->readUnsafe (userSizeInBytes, (u8 *)value);
rdtscll(stop);            
//printf("After bytesAvailable are: %d  %x %x %x %x \n", bytesAvailable, value[0], value[1], value[2], value[3] );

stop -= start;
        //
        // 3. check for errors
        //
        if (status != kNoError)
        {
            printf ("error: dma 1 read (%d)\n", status);
            break; 
        }

// --------------------------------------------------------------------------- OUTPUT

    board_0->DAC_Direct_Data[channel].writeRegister (flipflop);
    board_1->DAC_Direct_Data[channel].writeRegister (flipflop);
    flipflop *= -1;

if (stop > 30000) {
	printf("time %lld ", stop);
	printf("stat: %d joint_stat: %d joint_stat_2: %d\n", stat, joint_stat, joint_stat_2);
}
/*
	u16 ERRstatus = board->AI_Status_1.readRegister();
	if (ERRstatus != 0x10b0)
		printf("%hx\n", ERRstatus);
*/
        n++;
    }

    //
    //  Stop DMA process and disable DMA on the device
    //

    dma_0->stop ();
    dma_1->stop ();
    
    board_0->AI_AO_Select.setAI_DMA_Select (0);
    board_0->AI_AO_Select.flush ();
    board_1->AI_AO_Select.setAI_DMA_Select (0);
    board_1->AI_AO_Select.flush ();

	printf("14:FIFO_Full:%s_Half_Full:%s_Empty:%s Overrun:%sOverflow:%s "
               "SC_TC_Error:%sSTART2:%sSTART1:%sSC_TC:%sSTART:%sSTOP:%s "
               "G0_TC:%s_GateInterrupt:%s FIFO_Request:%s\n",
		board_0->AI_Status_1.readAI_FIFO_Full_St() ? "1" : "0",
		board_0->AI_Status_1.readAI_FIFO_Half_Full_St() ? "1" : "0",
		board_0->AI_Status_1.readAI_FIFO_Empty_St() ? "1" : "0",
		board_0->AI_Status_1.readAI_Overrun_St() ? "1" : "0",
		board_0->AI_Status_1.readAI_Overflow_St() ? "1" : "0",
		board_0->AI_Status_1.readAI_SC_TC_Error_St() ? "1" : "0",
		board_0->AI_Status_1.readAI_START2_St() ? "1" : "0",
		board_0->AI_Status_1.readAI_START1_St() ? "1" : "0",
		board_0->AI_Status_1.readAI_SC_TC_St() ? "1" : "0",
		board_0->AI_Status_1.readAI_START_St() ? "1" : "0",
		board_0->AI_Status_1.readAI_STOP_St() ? "1" : "0",
		board_0->AI_Status_1.readG0_TC_St() ? "1" : "0",
		board_0->AI_Status_1.readG0_Gate_Interrupt_St() ? "1" : "0",
		board_0->AI_Status_1.readAI_FIFO_Request_St() ? "1" : "0");
	printf("15:FIFO_Full:%s_Half_Full:%s_Empty:%s Overrun:%sOverflow:%s "
               "SC_TC_Error:%sSTART2:%sSTART1:%sSC_TC:%sSTART:%sSTOP:%s "
               "G0_TC:%s_GateInterrupt:%s FIFO_Request:%s\n",
		board_1->AI_Status_1.readAI_FIFO_Full_St() ? "1" : "0",
		board_1->AI_Status_1.readAI_FIFO_Half_Full_St() ? "1" : "0",
		board_1->AI_Status_1.readAI_FIFO_Empty_St() ? "1" : "0",
		board_1->AI_Status_1.readAI_Overrun_St() ? "1" : "0",
		board_1->AI_Status_1.readAI_Overflow_St() ? "1" : "0",
		board_1->AI_Status_1.readAI_SC_TC_Error_St() ? "1" : "0",
		board_1->AI_Status_1.readAI_START2_St() ? "1" : "0",
		board_1->AI_Status_1.readAI_START1_St() ? "1" : "0",
		board_1->AI_Status_1.readAI_SC_TC_St() ? "1" : "0",
		board_1->AI_Status_1.readAI_START_St() ? "1" : "0",
		board_1->AI_Status_1.readAI_STOP_St() ? "1" : "0",
		board_1->AI_Status_1.readG0_TC_St() ? "1" : "0",
		board_1->AI_Status_1.readG0_Gate_Interrupt_St() ? "1" : "0",
		board_1->AI_Status_1.readAI_FIFO_Request_St() ? "1" : "0");


// unloading data

    free(value);
    // --- Stop ----
    


    //
    // Reset DMA Channel and AI engine
    //
    dma_0->reset ();
    dma_1->reset ();
    
    aiReset(board_0);
    aiDisarm (board_0);
    aiReset(board_1);
    aiDisarm (board_1);

    //
    // Destroy register maps and address spaces
    //
    delete dma_0; 
    delete mite_0;
    delete board_0;
    delete dma_1; 
    delete mite_1;
    delete board_1;

    bus0->destroyAddressSpace(bar0_0);
    bus0->destroyAddressSpace(bar1_0);
    bus1->destroyAddressSpace(bar0_1);
    bus1->destroyAddressSpace(bar1_1);

	releaseBoard(bus1);

    return; 
} 
