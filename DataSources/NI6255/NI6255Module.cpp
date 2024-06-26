/*
 * author: Antonio Barbalace
 * notes: based on aiex1.cpp aoex1.cpp from National Instruments
 * date: 04-05 may 2010
 *  base version
 * rev: 23 july 2010
 *  added DMA support using nimhddk_linux26, directory hierical reworked
 * 
 * further notes: 
 *  class methods are in calling logical order
 * 
 * compiling:
 *  g++ -I module/nimhddk_linuxdevmem/ -I module/nimseries/ChipObjects/ -I module/nimseries/Library/ -c NI6255Module.cpp
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


#include "NI6255Module.h"

// -I /module/nimseries/Library/
#include "ai.h"
#include "ao.h"


/*
 * if your system lost FIFO elements
 * you need to realign the FIFO content
 * you can choose to realign reading the
 * - last sample (require a larger worst case reading time)
 * - oldest sample (require the smaller worst case reading time)
 */
#define REALIGN_FIFO_LAST
//#define REALIGN_FIFO_OLDEST


const int NI6255Module::kEepromSize = NI6255_EEPROM_SIZE;
const int NI6255Module::AI_CHANNELS = NI6255_AI_CHANNELS;
const int NI6255Module::AO_CHANNELS = NI6255_AO_CHANNELS;

NI6255Module::NI6255Module()
{
	bus = 0;
	bar0 = 0;
	bar1 = 0;
	board = 0;
	mite = 0;
	dma = 0;
	
	memset(eepromMemory, 0, kEepromSize);
	
	aiChannels = 0;
	aoChannels = 0;
	
	memset(AOscale, 0, sizeof(tScalingCoefficients) * AO_CHANNELS);
	memset(AIscale, 0, sizeof(tScalingCoefficients) * AI_CHANNELS);
	
	AIuserSizeInSamples = 0;
	AIuserSizeInBytes = 0;
	AIdmaSizeInSamples = 0;
	AIdmaSizeInBytes = 0;

	statistic_noBytesAvailable = 0;
	statistic_tooBytesAvailable = 0;
		
	boardIrq = 0;
	boardIrqfd = 0;
}

/* 
 * address = "PXI49::14::INSTR"
 */
int NI6255Module::Init (char* address)
{
    bus = acquireBoard(address);
    if(bus == NULL) {
		//printf("Could not access PCI device.  Exiting.\n");
		return -1;
	}
        
	// ---- Intitialise Mite Chip. ---
	
	//Skip MITE initialization for PCMCIA boards
	//(which do not have a MITE DMA controller)
	if(!bus->get(kIsPciPxiBus,0))
		goto continue_init;
	
	bar0 = bus->createAddressSpace(kPCI_BAR0);
	//Get the physical address of the DAQ board
//	physicalBar1 = bus->get(kBusAddressPhysical,kPCI_BAR1);
	physicalBar1 = bus->get(kBusAddressPhysical,kPCI_BAR1);
	//Tell the MITE to enable BAR1, where the rest of the board's registers are
//	bar0.write32(0xC0, (physicalBar1 & 0xffffff00L) | 0x80);
	bar0.write32(0xC0, (reinterpret_cast<unsigned long>(physicalBar1) & 0xffffff00L) | 0x80);
	//Destroy MITE address space (probably because the DMA code re-create it)
	bus->destroyAddressSpace(bar0);
	
	// ---- End of Intitialise Mite Chip. ---
	
continue_init:
    //  read eeprom for calibration information
    eepromReadMSeries (bus, eepromMemory, kEepromSize);   

    // create register map
    bar1 = bus->createAddressSpace(kPCI_BAR1);
    board = new tMSeries(bar1);
    
    return 1;
}

/*
 * Must be called only after NI6255Module::Init
 */
int NI6255Module::InitDMA ()
{
	if (!bar0)
		return -1;
	
	bar0 = bus->createAddressSpace(kPCI_BAR0);
	mite = new tMITE(bar0);
	mite->setAddressOffset(0x500);
	dma = new tDMAChannel(bus, mite);
	
	return 1;
}

void NI6255Module::Reset() 
{
	if (!board)
		return;
		
    // ---- AI/AO Reset ----
    //
    configureTimebase (board);
    pllReset (board);
    analogTriggerReset (board);

    // ---- AI Reset ----
    //
    aiReset (board);
    // check ai.h for aiPersonalize allowed values
    aiPersonalize (board, tMSeries::tAI_Output_Control::kAI_CONVERT_Output_SelectActive_Low);
    aiClearFifo (board);

    // ---- AO Reset ----
    //
    aoReset (board);
    aoPersonalize (board);
    aoResetWaveformChannels (board);
    aoClearFifo (board);

    // unground AO reference
    board->AO_Calibration.writeAO_RefGround (kFalse);
    
    // ADC reset only applies to 625x boards
    adcReset(board);

    // ---- End of AI/AO Reset ---- 
}

/*
 * what we want to configure?
 * - in channel
 *   - number
 *     - voltage range (gain)
 *     - polarity
 *     - type
 * - out channel number
 *   - number
 *     - voltage range (gain)
 *     - polarity
 *     - update
 */
int NI6255Module::Configure(int inputChannels, int outputChannels)
{
	if (!board)
		return -1;
	
	aiChannels = (inputChannels > AI_CHANNELS) ? AI_CHANNELS : inputChannels;
	aoChannels = (outputChannels > AO_CHANNELS) ? AO_CHANNELS : outputChannels;
	
	
    // ---- Start A0 task ---
	for (int i = 0; i<AO_CHANNELS; i++) {
		aoGetScalingCoefficients (eepromMemory, 0, 0, i, &(AOscale[i]));  
    
		aoConfigureDAC (board, 
                     i, 
                     0xF, 
                     tMSeries::tAO_Config_Bank::kAO_DAC_PolarityBipolar,
                     tMSeries::tAO_Config_Bank::kAO_Update_ModeImmediate);
	}
	// ---- End A0 task ----

	
    // ---- Start AI task ----
    aiDisarm (board);
    aiClearConfigurationMemory (board);
    // fill configuration FIFO 
    // Note: It is not necessary for the channel numbers to be ordered
    for (int i = 0; i < aiChannels; i++)
        aiConfigureChannel (board, 
                            i,  // channel number
                            1,  // gain -- check ai.h for allowed values
                            tMSeries::tAI_Config_FIFO_Data::kAI_Config_PolarityBipolar,
                            tMSeries::tAI_Config_FIFO_Data::kAI_Config_Channel_TypeNRSE,
                            (i == aiChannels-1)?kTrue:kFalse); // last channel?

    aiSetFifoRequestMode (board);    
    aiEnvironmentalize (board);
    aiHardwareGating (board);
    
    aiTrigger (board,
               tMSeries::tAI_Trigger_Select::kAI_START1_SelectPulse,
               tMSeries::tAI_Trigger_Select::kAI_START1_PolarityRising_Edge,
               tMSeries::tAI_Trigger_Select::kAI_START2_SelectPulse,
               tMSeries::tAI_Trigger_Select::kAI_START2_PolarityRising_Edge);
    
    aiSampleStop (board, 
                  (aiChannels > 1)?kTrue:kFalse); // multi channel?
    aiNumberOfSamples (board,   
                       1,      // posttrigger samples
                       0,      // pretrigger samples
                       kTrue); // continuous?
    // using external START signal
    exportSignal(board, kAIStartTrigger,kPFI0, kFalse);
    aiSampleStart (board, 
                   0,
                   0,
                   tMSeries::tAI_START_STOP_Select::kAI_START_SelectPFI0,
                   tMSeries::tAI_START_STOP_Select::kAI_START_PolarityRising_Edge);
    // export convert on kPFI1
    exportSignal(board, kAIConvertClock,kPFI1, kTrue);
    aiConvert (board, 
               28,     // convert period divisor
               28,     // convert delay divisor 
               kTrue); // external sample clock?
    
    aiClearFifo (board);
        
	for (int i = 0; i<AI_CHANNELS; i++) {
	    // check scale.h for scaling index values
	    aiGetScalingCoefficients (eepromMemory, 0, 0, i, &(AIscale[i]));  
	}
	// ---- End AI task ----
		
	return 1;
}

/*
 * what we want to configure?
 * - DMA in buffer
 *   - number of byte
 *   - implementation (ring, linear...)
 *   - data width
 * - DMA out buffer
 *   - number of byte
 *   - implementation (ring, linear...)
 *   - data width
 */
int NI6255Module::ConfigureDMA()
{
	tDMAError status = kNoError;
	
	AIuserSizeInSamples =  aiChannels; 
	AIuserSizeInBytes   =  AIuserSizeInSamples * sizeof(unsigned short); 	
	AIdmaSizeInSamples  =  DMA_AI_SAMPLES * aiChannels; 
    AIdmaSizeInBytes    =  AIdmaSizeInSamples * sizeof(unsigned short); 
	
	// configure DMA on the device (board)
	board->AI_AO_Select.setAI_DMA_Select(1);
	board->AI_AO_Select.flush();

    //
	// Configure and start DMA Channel
	//
	//    DMA operations use bytes instead of samples.
	//    622x and 625x devices transfer 16-bits at a time (1 sample)
	//    628x devices transfer 32-bits at a time (1 sample)
	//    Start the DMA engine before arming/starting the AI engine
	//

	status = dma->config (0, tDMAChannel::kRing, tDMAChannel::kIn, AIdmaSizeInBytes, tDMAChannel::k16bit);
	if (status != kNoError) // DMA configuration error
		return -1;

	return 1;
}

int NI6255Module::InitIRQ (int irq)
{
	if (!board)
		return -1;
	
	// ---- Start IRQ task ---- 
	if (irq) {
		char path[128];
		sprintf(path, "/proc/nirlpk/%d", irq);
		boardIrq = irq;
		// open device driver
		boardIrqfd = open(path, O_RDONLY);
		if (boardIrqfd < 0)
			return -1;

		// signal a configuration start to the board subsystem
        board->Joint_Reset.setAI_Configuration_Start (1);
        board->Joint_Reset.flush();

		// reset the interrupt controller register (no interrupt generation)
        board->Interrupt_Control.writeRegister(0);
		board->Interrupt_Control.flush();
		// enable group A interrupt generation
		board->Interrupt_Control.setInterrupt_Group_A_Enable(1);
		board->Interrupt_Control.flush();
        // START interrupt enabled
	    board->Interrupt_A_Enable.writeAI_START_Interrupt_Enable(1);
		//board->Interrupt_A_Enable.writeAI_STOP_Interrupt_Enable(1);
		board->Interrupt_A_Enable.flush();

		// signal a configuration end to the board subsystem
		board->Joint_Reset.setAI_Configuration_End (1);
		board->Joint_Reset.flush();
	}
    // ---- End IRQ task ----
	
	return 1;
}

/*
 * First start the DMA engine and then the  acquisition process (NI6255Module::Start())
 */
int NI6255Module::StartDMA ()
{
	if (!dma)
		return -1;
	
	tDMAError status = kNoError;
    status = dma->start();
    if ( kNoError != status )
        return -2;
    else 
    	return  1;
}

int NI6255Module::Start ()
{
	if (!board)
		return -1;
	
    aiArm (board, kFalse); 
    aiStart (board);
    return 1;
}

void NI6255Module::AckIRQ ()
{
	// ack interrupts
	board->Interrupt_A_Ack.setAI_START_Interrupt_Ack(1);
	//board->Interrupt_A_Ack.setAI_STOP_Interrupt_Ack(1);
	board->Interrupt_A_Ack.flush();
	
	// re-enable interrupts
	//board->Interrupt_A_Enable.writeAI_START_Interrupt_Enable(1);
	//board->Interrupt_A_Enable.writeAI_STOP_Interrupt_Enable(1);
	//board->Interrupt_A_Enable.flush();
}

int NI6255Module::WaitIRQ ()
{
	int nints;
	if (!board)
		return -1;
	
	// START
	while( read(boardIrqfd, &nints, sizeof(nints)) != sizeof(nints) )
	{
		// waiting on the blocking read (for a START signal)
	}
	AckIRQ();
	
    // STOP
	while ( board->AI_Status_1.readAI_FIFO_Empty_St() )
    {
        // waiting for something in the FIFO
    }
	return nints;
}

/*
 * the method returns bytesAvailable in the DMA buffer 
 * userSizeinBytes is initilized in ConfigureDMA
 */
//#include <sched.h>
static int first = 1;
int NI6255Module::PollDMA ()
{
	//unsigned int bytesAvailable = 0;
	bytesAvailable = 0;
	tDMAError status = kNoError;
	
	if (!dma)
		return -1;
/*
if (first) {
printf("\nSONNO\n\n");
   sleep(10);
   first = 0;
}
*/	
    do 
    {
        //
        //  1. Use read to query the number of bytes in the DMA buffers
        //
//sched_yield();
    	status = dma->read(0, NULL, (u32*)(&bytesAvailable));
    }  while ( (bytesAvailable < AIuserSizeInBytes) && (kNoError == status));
    
    return bytesAvailable;
}

void NI6255Module::Poll()
{
	// START
	while ( !board->Joint_Status_2.readAI_Scan_In_Progress_St() &&
			board->AI_Status_1.readAI_FIFO_Empty_St() )
	{
		// polling for Scan_In_Progress (for a START signal)
	}
	
    // STOP
	while ( board->Joint_Status_2.readAI_Scan_In_Progress_St() &&
	     board->AI_Status_1.readAI_FIFO_Empty_St() )
    {
        // waiting for scan to complete (for a STOP signal)
    }
}

/*
 * count is the number of u16 in the buffer
 */
int NI6255Module::Read (u16 *buffer, int count)
{
	u16 value;
	if(!board)
		return -1;

#ifdef REALIGN_FIFO_LAST
	do {
#endif
	// flush FIFO
	int m;
	for (m = 0; m < aiChannels; m++)
	{
		// the FIFO must be empty
		value = board->AI_FIFO_Data.readRegister();
		// values are copyed only if there is space in the buffer
		if ( m < count)
			buffer[m] = value;
	}
#ifdef REALIGN_FIFO_LAST
	} while ( !board->AI_Status_1.readAI_FIFO_Empty_St() );
#elif defined  REALIGN_FIFO_OLDEST
    aiClearFifo (board);
#endif
    
	return count;
}

/*
 * count is the number of u32 in the buffer
 */
int NI6255Module::Read (u32 *buffer, int count)
{
	u32 value;
	if(!board)
		return -1;

#ifdef REALIGN_FIFO_LAST
	do {
#endif
	// flush FIFO
	int m;
	for (m = 0; m < aiChannels; m++)
	{
		// the FIFO must be empty
		value = board->AI_FIFO_Data.readRegister();
		// values are copyed only if there is space in the buffer
		if ( m < count)
			buffer[m] = value;
	}
#ifdef REALIGN_FIFO_LAST
	} while ( !board->AI_Status_1.readAI_FIFO_Empty_St() );
#elif defined  REALIGN_FIFO_OLDEST
    aiClearFifo (board);
#endif
    
	return count;
}

/*
 * count is the number of f32 in the buffer
 */
int NI6255Module::Read (f32 *buffer, int count)
{
	i32 value;
	f32 fvalue;
	if(!board)
		return -1;

#ifdef REALIGN_FIFO_LAST
	do {
#endif
	// flush FIFO
	int m;
	for (m = 0; m < aiChannels; m++)
	{
		// the FIFO must be empty
		value = board->AI_FIFO_Data.readRegister();
		// values are copyed only if there is space in the buffer
		if ( m < count) {
			aiPolynomialScaler (&value, &fvalue, &(AIscale[m]));
			buffer[m] = fvalue;
		}
	}
#ifdef REALIGN_FIFO_LAST
	} while ( !board->AI_Status_1.readAI_FIFO_Empty_St() );
#elif defined REALIGN_FIFO_OLDEST
    aiClearFifo (board);
#endif
	
    return count;
}

/*
 * return at maximum userSizeInBytes bytes because the read
 * is unsafe and we where polling for userSizeInBytes in the DMA buffer.
 */
int NI6255Module::ReadDMA(u32 *buffer, int count)
{
	int bytes = (AIuserSizeInBytes < (count*sizeof(unsigned short))) ?
		AIuserSizeInBytes : (count*sizeof(unsigned short));
	tDMAError status = kNoError;
	
	if (!dma)
		return -1;
	
/*    
///////////////GABRIELE PROVA
     u32 bytesAvailable;
     do {
	status = dma->readUnsafe (bytes, (u8 *)buffer);
    	status = dma->read(0, NULL, &bytesAvailable);
    }while ( bytesAvailable > 0);

    return 1;
///////////////////////////
*/

   //if the board is not used as the TimeInput module has done no check, do it
/*   if (bytesAvailable < AIuserSizeInBytes) {
      statistic_noBytesAvailable++;
      PollDMA();
   }
*/
/*
#ifdef REALIGN_FIFO_LAST
last_tooBytesAvailable = statistic_tooBytesAvailable;
do {
#endif
*/

if ( bytesAvailable <  AIuserSizeInBytes)
	statistic_noBytesAvailable++;

    status = dma->readUnsafe (bytes, (u8 *)buffer);

//statistics are required
bytesAvailable -= bytes;
if ( bytesAvailable >  AIuserSizeInBytes)
	statistic_tooBytesAvailable++;

//expand 16 to 32 bit
for (int i = (count -1); i > -1; i--)
	buffer[i] = (unsigned int)(int)(((short*)(buffer))[i]);

/*
    bytesAvailable -= bytes;
    statistic_tooBytesAvailable++;
 
#ifdef REALIGN_FIFO_LAST
} while ( (bytesAvailable >= AIuserSizeInBytes) );
last_tooBytesAvailable = statistic_tooBytesAvailable - last_tooBytesAvailable;
#endif
    statistic_tooBytesAvailable--;
*/
    //
    // 3. check for errors
    //
    if (status != kNoError)
    	return -1;
    else 
    	return 1;
}

/*
 * double buffered (with scaling coefficients)
 * is unsafe and we where polling for userSizeInBytes in the DMA buffer.
 */
int NI6255Module::ReadDMA(f32 *buffer, int count)
{
	i32 value;
	f32 fvalue;
	tDMAError status = kNoError;
	
	if (!dma)
		return -1;
	
    status = dma->readUnsafe (AIuserSizeInBytes, (u8 *)AIdmaUser);
    
	int m;
	for (m = 0; m < aiChannels; m++)
		if ( m < count) {
			value = AIdmaUser[m];
			aiPolynomialScaler (&value, &fvalue, &(AIscale[m]));
			buffer[m] = fvalue;
		}
  
    //
    // 3. check for errors
    //
    if (status != kNoError)
    	return -1;
    else 
    	return ((count < AIuserSizeInSamples) ? count : AIuserSizeInSamples);
}

void NI6255Module::Write (u16 *buffer, int count)
{
	if (!board)
		return;
	int m;
	for (m = 0; m < aoChannels; m++)
		if (m < count) {
			board->DAC_Direct_Data[m].writeRegister (buffer[m]);
		}
}

void NI6255Module::Write (u32 *buffer, int count)
{
	if (!board)
		return;
	int m;
	for (m = 0; m < aoChannels; m++)
		if (m < count) {
			board->DAC_Direct_Data[m].writeRegister (buffer[m]);
		}
}

void NI6255Module::Write (f32 *buffer, int count)
{
	if (!board)
		return;
	i32 value;
	int m;
	for (m = 0; m < aoChannels; m++)
		if (m < count) {
			aoLinearScaler (&value, &(buffer[m]), &(AOscale[m]));
			board->DAC_Direct_Data[m].writeRegister (value);
		}
}

void NI6255Module::StopDMA ()
{
	if (!dma)
		return;
	
	dma->stop ();
	board->AI_AO_Select.setAI_DMA_Select(0);
	board->AI_AO_Select.flush();
}

void NI6255Module::Stop ()
{
	if (!board)
		return;
	
	aiDisarm (board);	
}

/*
 * must be called before NI6255Module::Release()
 */
void NI6255Module::ReleaseDMA ()
{
	if (!dma)
		return;
	
	dma->reset();
	delete dma;
	dma = 0;
	
	if (mite) {
		// cleanup
		delete mite;
		mite = 0;
	}
	
	if (bus) {
		bus->destroyAddressSpace(bar0);
		bar0 = 0;
	}
}

void NI6255Module::ReleaseIRQ ()
{
	if (!board)
		return;
	
	if (boardIrq) {
		// signal a configuration start to the board subsystem
		board->Joint_Reset.setAI_Configuration_Start (1);
		board->Joint_Reset.flush();

		// reset the interrupt controller register (no interrupt generation)
		board->Interrupt_Control.writeRegister(0);
		board->Interrupt_Control.flush();
		// enable group A interrupt generation
		board->Interrupt_Control.setInterrupt_Group_A_Enable(1);
		board->Interrupt_Control.flush();
		// START interrupt enabled
		board->Interrupt_A_Enable.writeAI_START_Interrupt_Enable(1);
		//board->Interrupt_A_Enable.writeAI_STOP_Interrupt_Enable(1);
		board->Interrupt_A_Enable.flush();

		// signal a configuration end to the board subsystem
		board->Joint_Reset.setAI_Configuration_End (1);
		board->Joint_Reset.flush();
		boardIrq = 0; 
	}
	
	if (boardIrqfd) {
		// close file descriptor
	    close(boardIrqfd);
	    boardIrqfd = 0;
	}
}

void NI6255Module::Release()
{
	if (board) {
		// cleanup
		delete board;
		board = 0;
	}
    
	if (bus) {
		bus->destroyAddressSpace(bar1);
		releaseBoard(bus);
		bus = 0;
		bar1 = 0;
	}
}

void NI6255Module::GetStatistics(unsigned int * no, unsigned int * too, unsigned int* last)
{
	if (no)
		*no = statistic_noBytesAvailable;
	if (too)
		*too = statistic_tooBytesAvailable;
	if (last)
		*last = last_tooBytesAvailable;

}


NI6255Module::~NI6255Module()
{
	StopDMA();
	Stop();
	
	ReleaseIRQ();
	Release();
}
