//
//  aiex1.cpp --
//
//      On-demand acquisition
//	MDSplus support for data recording
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

#include <mdslib.h>
#define statusOk(status)((status) & 1) 

int write(float* DataArray, int channels, int samples);

void test(iBus *bus)
{

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



    u32 numberOfSamples = (8192);    
    u32 numberOfChannels = 64;  
    
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
    // ai clear FIFO
    aiClearFifo (board);
    aiClearFifo (board);
    aiClearFifo (board);
    
    // ADC reset only applies to 625x boards
    adcReset(board);
    adcReset(board);
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
/*                            tMSeries::tAI_Config_FIFO_Data::kAI_Config_PolarityUnipolar,*/
                            tMSeries::tAI_Config_FIFO_Data::kAI_Config_PolarityBipolar,
                            tMSeries::tAI_Config_FIFO_Data::kAI_Config_Channel_TypeNRSE, 
/*                            tMSeries::tAI_Config_FIFO_Data::kAI_Config_Channel_TypeDifferential, */
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
/*               28,     // convert period divisor
               28,     // convert delay divisor
*//*               24,     // convert period divisor
               24,     // convert delay divisor 
*/               20,     // convert period divisor
               20,     // convert delay divisor  
             kFalse); // external sample clock?

    tScalingCoefficients scale;
    
    // check scale.h for scaling index values
    aiGetScalingCoefficients (eepromMemory, 0, 0, 0, &scale);  
    
    // ---- Read FIFO ----
    
    i32 value;
    f32 scaled;
    int n = 0; // number of samples counter 
    int m = 0; // number of channels counter

    // output array
    f32* data_array = (f32*) malloc(numberOfSamples * numberOfChannels * sizeof(f32));
    //i32* data_array = (i32*) malloc(numberOfSamples * numberOfChannels * sizeof(i32));
if (data_array == 0) {
    printf("malloc fail");
    return ;
}



   aiClearFifo (board);
   aiClearFifo (board);

    aiArm (board, kFalse); 
    aiStart (board);


    //printf("go\n");
    while (n < numberOfSamples)
    {
        // trigger a scan
        //aiStartOnDemand (board); 
   // printf("go %d\n", n);

/* questo codice va bene fino a 3kHz circa poi non ce la si fa piu!!!
	while ( !board->Joint_Status_2.readAI_Scan_In_Progress_St() &&
	     board->AI_Status_1.readAI_FIFO_Empty_St())
        {
            // waiting for scan to complete
	    //joint_stat++;
        }   
        
        while ( board->Joint_Status_2.readAI_Scan_In_Progress_St())
        {
            // waiting for scan to complete
        }
*/
        while(board->AI_Status_1.readAI_FIFO_Empty_St())
	{
	    // waiting something in the fifo
        }
//printf("\n --- scan finished --- \n");

        m = 0;
        //while (!(board->AI_Status_1.readAI_FIFO_Empty_St()) && (m < numberOfChannels))
	while (m < numberOfChannels)
        {
           // if(!board->AI_Status_1.readAI_FIFO_Empty_St())
           // {
                value = board->AI_FIFO_Data.readRegister ();
                aiPolynomialScaler (&value, &scaled, &scale);
                //printf ("%d::%e,\n", m, scaled);
		data_array[m*numberOfSamples + n] = scaled;
		//data_array[m*numberOfSamples + n] = value;
                m++;
           // }
	   // else
	//	printf("error m = %d n = %d \n", m,n);
        }

        n++;

    }

	m = 0;
        while(!board->AI_Status_1.readAI_FIFO_Empty_St()) {
		value = board->AI_FIFO_Data.readRegister ();
		m++;
//		printf("over data\t");
        }
	if (m > 0)
	  printf("over data %d\n", m);


/*
    for (n=0; n<numberOfSamples; n++) {
        printf("\n");
        for (m=0; m< numberOfChannels; m++) {
            printf("%e\t ", data_array[m*numberOfSamples + n]);
            //printf("%8d\t ", data_array[m*numberOfSamples + n]);
        }
    }
*/    
printf("\nflushing to MDSplus\n");
    write(data_array, numberOfChannels, numberOfSamples);
    free(data_array);

    // --- Stop ----
    
    aiDisarm (board);
  aiDisarm (board);
  aiDisarm (board);
      

    // cleanup
    delete board;
    bus->destroyAddressSpace(bar1);

    return; 
} 

/*
test gabriele sfasamenti channels
ch 0,2,4,6,8,10,12,14,62 sinusoidal
ch 1,3,5,7,9,11,13,15,63 ground (sense)
1.4us conversion time
shot 100 - clock 5kHz (circa) - sin 1kHz
shot 101 - clock 5kHz (circa) - sin 800Hz
shot 102 - clock 5kHz (circa) - sin 600Hz
shot 103 - clock 5kHz (circa) - sin 400Hz
shot 104 - clock 5kHz (circa) - sin 200Hz
1.2us conversion time
shot 110 - clock 5kHz (circa) - sin 1kHz
shot 111 - clock 5kHz (circa) - sin 800Hz
shot 112 - clock 5kHz (circa) - sin 600Hz
shot 113 - clock 5kHz (circa) - sin 400Hz
shot 114 - clock 5kHz (circa) - sin 200Hz
1.0us conversion time
shot 120 - clock 5kHz (circa) - sin 1kHz
shot 121 - clock 5kHz (circa) - sin 800Hz
shot 122 - clock 5kHz (circa) - sin 600Hz
shot 123 - clock 5kHz (circa) - sin 400Hz
shot 124 - clock 5kHz (circa) - sin 200Hz
*/

#define HOST_ADDR "150.178.32.45:8000"
#define TREE_NAME "test"
#define SHOT_NUM 124

int write(float* DataArray, int channels, int samples) {
	int dtypeFloat = DTYPE_FLOAT; /* We are going to write a signal made of float values */
	int null = 0; /* Used to mark the end of the argument list */
	int status; /* Will contain the status of the data access operation */
	int socket; /* Will contain the handle to the remote mdsip server */
	int dataDesc; /* Data descriptor */
	int timeDesc; /* Timebase descriptor */
	int shot = SHOT_NUM; /* Just an example shot number */
	int len; /* The dimension of X and Y arrays */
	int i;

	float* timebase; /* Array of floats used for timebase */

	/* Connect to MDSplus */
printf("\nMds connecting to %s", HOST_ADDR);
	socket = MdsConnect(HOST_ADDR);
	if ( socket == -1 )
	{
		fprintf(stderr,"Error connecting to mdsip server.\n");
		return -1;
	}

	/* Open tree */
printf("\nMds open tree %s", TREE_NAME);
	status = MdsOpen(TREE_NAME, &shot);
	if ( !statusOk(status) )
	{
		fprintf(stderr,"Error opening tree for shot %d: %s.\n",shot/*, MdsGetMsg(status)*/);
		return -1;
	}

	/* create a time base */
	timebase = (float*) malloc(samples * sizeof(float));
	for(i = 0; i < samples; i++)
		timebase[i] = i/((float)samples); /*Build a signal which lasts 1 sec */

	/* Build timebase descriptor */
	len = samples; /* number of samples */
	timeDesc = descr(&dtypeFloat, timebase, &len, &null);

printf("\nMds flushing data\n");
char channelName[64];
for (i=0; i< channels; i++) {
	/* Build data descriptor */
	dataDesc = descr(&dtypeFloat, &(DataArray[i*samples]), &len, &null);
	sprintf(channelName, "SIG%d:DATA", i);

	/* Write each signal */
	status = MdsPut(channelName, "BUILD_SIGNAL($1,,$2)", &dataDesc, &timeDesc, &null);
	if ( !statusOk(status) )
	{
		/* error */
		fprintf(stderr,"Error writing signal: %s\n"/*, MdsGetMsg(status)*/);
		return -1;
	}
}

	/* Close tree */
	status = MdsClose(TREE_NAME, &shot);
	if ( !statusOk(status) )
	{
		fprintf(stderr,"Error closing tree for shot %d: %s.\n",shot /*,MdsGetMsg(status)*/ );
		return -1;
	}
	/* Done */
	return 0;
}

/* 
make clean
make
g++ -lMdsLib -lMdsIpShr aiex1.o osiBus.o osiUserCode.o tTIO.o tMSeries.o ai.o ao.o common.o scale.o main.o tDMAChannel.o tLinearDMABuffer.o tDMABuffer.o tMITE.o -oaiex1
*/


