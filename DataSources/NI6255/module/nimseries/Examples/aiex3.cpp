//
//  aiex3.cpp
//
//     Analog Input - Hardware timed acquisition with DMA
//
//  $DateTime: 2006/10/26 14:11:17 $
//
#include "tMSeries.h"
#include "ai.h"
#include "common.h"
#include "scale.h"

// dma support
#include "nimhddk_dma/tDMAChannel.h"
#include "nimhddk_dma/tMITE.h"

#include <stdio.h>

void test(iBus *bus)
{
    const u32 numberOfChannels = 80;
    const u32 numberOfSamples = 10000;
    const u32 samplePeriodDivisor = 200; // timebase/sample rate => 20 MHz / 100 kHz
    
    const tBoolean continuous = kFalse; 
    tDMAError status = kNoError; 
   
    //
    // Specify DMA and user buffer sizes
    //

    const u32 dmaSizeInSamples = 100000; 
    const u32 dmaSizeInBytes   = dmaSizeInSamples * sizeof(i16); 

    const u32 userSizeInSamples = 10000; 
    const u32 userSizeInBytes   = userSizeInSamples * sizeof(i16);

    i16 *rawData = new i16[userSizeInSamples];
    
    const u32 sampleSizeInBytes = sizeof(i16);
   
    //
    //  Register Map objects
    //
    
    tAddressSpace Bar1;
    tMSeries     *board;
    
    Bar1 = bus->createAddressSpace(kPCI_BAR1);
    
    board  = new tMSeries(Bar1);

    //
    // DMA objects
    //
    tAddressSpace bar0;
    tMITE       *mite;
    tDMAChannel *dma;

    bar0 = bus->createAddressSpace(kPCI_BAR0);
    mite = new tMITE(bar0);
    mite->setAddressOffset(0x500);
    
    dma = new tDMAChannel(bus, mite);
    
    //
    //  read eeprom for calibration information
    //
    
    const u32 kEepromSize = 1024;
    u8 eepromMemory[kEepromSize];
    eepromReadMSeries (bus, eepromMemory, kEepromSize);   

    //
    // program device
    //
    
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
    
    for (u32 i = 0; i < numberOfChannels; i++)
    {        
        aiConfigureChannel (board, 
                            i,  // channel number
                            1,  // gain -- check ai.h for allowed values
                            (i < 64 ? tMSeries::tAI_Config_FIFO_Data::kAI_Config_PolarityBipolar : tMSeries::tAI_Config_FIFO_Data::kAI_Config_PolarityUnipolar),
                            tMSeries::tAI_Config_FIFO_Data::kAI_Config_Channel_TypeRSE, 
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
                       numberOfSamples, // posttrigger samples
                       0,               // pretrigger samples
                       continuous);     // continuous?
    
    aiSampleStart (board, 
                   samplePeriodDivisor, 
                   3, 
                   tMSeries::tAI_START_STOP_Select::kAI_START_SelectSI_TC,
                   tMSeries::tAI_START_STOP_Select::kAI_START_PolarityRising_Edge);
    
    aiConvert (board, 
               280,     // convert period divisor
               3,       // convert delay divisor 
               kFalse); // external sample clock?
    
    aiClearFifo (board);
    
    tScalingCoefficients scale;
    
    // check scale.h for scaling index values
    aiGetScalingCoefficients (eepromMemory, 0, 0, 0, &scale);
    
    //
    // Configure DMA on the device
    //
    board->AI_AO_Select.setAI_DMA_Select (1);
    board->AI_AO_Select.flush ();

    //
    // Configure and start DMA Channel
    //
    //    DMA operations use bytes instead of samples.
    //    622x and 625x devices transfer 16-bits at a time (1 sample)
    //    628x devices transfer 32-bits at a time (1 sample)
    //    Start the DMA engine before arming/starting the AI engine
    //
    
    status = dma->config (0, tDMAChannel::kRing, tDMAChannel::kIn, dmaSizeInBytes, tDMAChannel::k16bit);
    if (status != kNoError)
    {
        printf ("Error: dma configuration (%d)\n", status);
    }
    
    status = dma->start();
    if ( kNoError != status )
    {
        printf ("Error: dma start (%d)\n", status);
    }
    else
    {
        //
        // No error - arm and start AI engine
        //
        aiArm (board, kTrue);
        aiStart (board);
    }
    
    //
    // DMA Read
    //
    
    u32 totalNumberOfBytes = numberOfSamples * numberOfChannels * sampleSizeInBytes;

    u32 bytesRead = 0;      // bytes read so far
    u32 bytesAvailable = 0; // bytes in the DMA buffer
    u32 tries = 0;          // number of tries without reading

    //
    // Setup File Logging
    //

    FILE * pFile;

    pFile = fopen ("AIDMA.log","w");

    while ( (bytesRead < totalNumberOfBytes) && (kNoError == status))
    {
        //
        //  1. Use read to query the number of bytes in the DMA buffers
        //
        status = dma->read(0, NULL, &bytesAvailable);

        //
        //  2. if theres enough data, read
        //
        if ( bytesAvailable >= userSizeInBytes )
        {
            status = dma->read (userSizeInBytes, (u8 *)rawData, &bytesAvailable);
            
            // process & scale data - print first 10 points of the buffer
            for (u32 i=0; i<(userSizeInBytes / sampleSizeInBytes); ++i)
            {
                f32 scaled;
                i32 raw = (i32) rawData[i]; 
               
                aiPolynomialScaler ( &raw , &scaled, &scale);
                fprintf (pFile, "%d: %f\n", (i % numberOfChannels), scaled );
            }

            bytesRead += userSizeInBytes; 
            
            // reset timeout
            tries = 0; 

            printf ("bytesAvailable: %d\n", bytesAvailable);
            printf ("bytesRead: %d\n", bytesRead);
        }

        //
        // 3. check for errors
        //
        if (status != kNoError)
        {
            printf ("error: dma read (%d)\n", status);
            break; 
        }

        // sleep to reduce cpu usage
        
        //
        //  timeout - if tried more than 10000 without reading, exit.
        //
        ++tries;
        if (tries == 100000)
        {
            printf ("dma read timeout!\n");
            status = kDataNotAvailable;
            break;
        }
    }

    // Close File

    fclose (pFile);

    //
    //  Stop DMA process and disable DMA on the device
    //

    dma->stop ();
    
    board->AI_AO_Select.setAI_DMA_Select (0);
    board->AI_AO_Select.flush ();
    
    // cleanup

    //
    // Delete user buffer
    //
    delete [] rawData; 

    //
    // Reset DMA Channel and AI engine
    //
    dma->reset ();
    
    aiReset(board);
    
    //
    // Destroy register maps and address spaces
    //
    delete dma; 
    delete mite;
    delete board;

    bus->destroyAddressSpace(bar0);
    bus->destroyAddressSpace(Bar1);
}
