/*
 * author: Antonio Barbalace
 * notes: based on aiex1.cpp aoex1.cpp from National Instruments
 * date: ?!?!
 *  base version
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


#include "NI6723Module.h"
#include <ioctl.h>
#include <errno.h>

#include "ao.h"

#include <sys/ioctl.h>
const int NI6723Module::kEepromSize = NI6723_EEPROM_SIZE;
const int NI6723Module::AI_CHANNELS = NI6723_AI_CHANNELS;
const int NI6723Module::AO_CHANNELS = NI6723_AO_CHANNELS;

NI6723Module::NI6723Module()
{
	bus = 0;
	bar0 = 0;
	bar1 = 0;
	board = 0;
//	mite = 0;
//	dma = 0;
	
	memset(eepromMemory, 0, kEepromSize);
	
	aiChannels = 0;
	aoChannels = 0;
	
//	memset(AOscale, 0, sizeof(tScalingCoefficients) * AO_CHANNELS);
//	memset(AIscale, 0, sizeof(tScalingCoefficients) * AI_CHANNELS);
	memset(AOscale_offset, 0, sizeof(double) * AO_CHANNELS);
	memset(AOscale_gain, 0, sizeof(double) * AO_CHANNELS);

// TODO eliminate after tests
pippo=0x07ff;
	
	boardIrq = 0;
	boardIrqfd = 0;
}

/* 
 * address = "PXI49::14::INSTR"
 */
int NI6723Module::Init (char* address)
{
  
    iBus *bus;

    unsigned long physicalBar0;
    unsigned long physicalBar1;

    void *mem0 = NULL;
    void *mem1 = NULL;
    
    u32 busNumber = 0; 
    u32 devNumber = 0; 
    i32 brdId = 0; 
   

    int fd = 0;
    char file[32];
/*    
    sprintf(file, "/dev/nirlpk%d", brdId);
    
    printf("APRO FILE %s\n", file);
    fd = open (file, O_RDWR);
    if (fd <= 0 )
    {
        printf ("fail to open %s %d %s\n", file, fd, strerror(errno));
        return NULL;
    }

    printf("FILE APERTO  %X\n", NIRLP_IOCTL_GET_PHYSICAL_ADDRESS);
    sleep(1);
    
    physicalBar0 = 0; 
    if ( ioctl (fd, NIRLP_IOCTL_GET_PHYSICAL_ADDRESS, &physicalBar0) < 0 )
    {
   printf("IOCTL FALLITA\n");
    sleep(1);
    //   close(fd);
       return NULL;
    }
    printf("PRIMO IOCTL FATTO\n");
    sleep(1);
    
    physicalBar1 = 1; 
    if ( ioctl (fd, NIRLP_IOCTL_GET_PHYSICAL_ADDRESS, &physicalBar1) < 0 )
    {
       close(fd);
       return NULL;
    }
    printf("SECONDO IOCTL FATTO\n");

  
  return 0;
  
  
  
  
  
 */ 
  
  
  printf("ACQUIRING BOARD %s\n", address);
  sleep(1);
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
	
printf("HAS MITE\n");
	bar0 = bus->createAddressSpace(kPCI_BAR0);
	//Get the physical address of the DAQ board
	physicalBar1 = bus->get(kBusAddressPhysical,kPCI_BAR1);
	//Tell the MITE to enable BAR1, where the rest of the board's registers are
	bar0.write32(0xC0, (physicalBar1 & 0xffffff00L) | 0x80);
	//Destroy MITE address space (probably because the DMA code re-create it)
	bus->destroyAddressSpace(bar0);
	
sleep(1);
	// ---- End of Intitialise Mite Chip. ---
	
continue_init:

	// ---- Program 6723 FPGA chip ----

	test (bus);

	// ---- End of Program 6723 FPGA chip ----

    // create register map
    bar1 = bus->createAddressSpace(kPCI_BAR1);
    board = new t6723(bar1);
    
    // Check whether FPGA downloaded correctly
    u32 check = 0, version = 0;
    version = board->FPGAVersion.read();
    printf("Expected FPGA version: 04111813 --- Actual FPGA version: %08X\n", version);
    check = board->FPGACheck.read();
    printf("Expected FPGA check: 36373233 --- Actual FPGA check = %X\n", check);
    
    if ( version != 0x4111813 || check != 0x36373233 ) {
      //printf("FPGA version check failed. Please verify that it has been properly\nloaded with initialize6723.\n");
 //     return -1;   Gabriele Commentato.........
    }
    
    // registering pointers to DirectData classes
    DirectDataArray[0] = &(board->DirectData_00);
    DirectDataArray[1] = &(board->DirectData_01);
    DirectDataArray[2] = &(board->DirectData_02);
    DirectDataArray[3] = &(board->DirectData_03);
    DirectDataArray[4] = &(board->DirectData_04);
    DirectDataArray[5] = &(board->DirectData_05);
    DirectDataArray[6] = &(board->DirectData_06);
    DirectDataArray[7] = &(board->DirectData_07);
    DirectDataArray[8] = &(board->DirectData_08);
    DirectDataArray[9] = &(board->DirectData_09);
    
    DirectDataArray[10] = &(board->DirectData_10);
    DirectDataArray[11] = &(board->DirectData_11);
    DirectDataArray[12] = &(board->DirectData_12);
    DirectDataArray[13] = &(board->DirectData_13);
    DirectDataArray[14] = &(board->DirectData_14);
    DirectDataArray[15] = &(board->DirectData_15);
    DirectDataArray[16] = &(board->DirectData_16);
    DirectDataArray[17] = &(board->DirectData_17);
    DirectDataArray[18] = &(board->DirectData_18);
    DirectDataArray[19] = &(board->DirectData_19);

    DirectDataArray[20] = &(board->DirectData_20);
    DirectDataArray[21] = &(board->DirectData_21);
    DirectDataArray[22] = &(board->DirectData_22);
    DirectDataArray[23] = &(board->DirectData_23);
    DirectDataArray[24] = &(board->DirectData_24);
    DirectDataArray[25] = &(board->DirectData_25);
    DirectDataArray[26] = &(board->DirectData_26);
    DirectDataArray[27] = &(board->DirectData_27);
    DirectDataArray[28] = &(board->DirectData_28);
    DirectDataArray[29] = &(board->DirectData_29);

    DirectDataArray[30] = &(board->DirectData_30);
    DirectDataArray[31] = &(board->DirectData_31);

    // Reading the calibration data into the array
    u16 addr = 64;
    for (int j = 0; j < AO_CHANNELS; j++)
    {
      u32 gainInt = 0;
      u32 offsetInt = 0;
      
      // reading GAIN
      for (int i=0;i<4;i++)
        gainInt = (gainInt << 8) + readFromEEPROM(board, addr + i);
      AOscale_gain[j] = u32ToDouble(gainInt);
      addr += 4;
      
      // reading OFFSET
      for (int i=0;i<4;i++)
        offsetInt = (offsetInt << 8) + readFromEEPROM(board, addr + i);
      AOscale_offset[j] = u32ToDouble(offsetInt);
      addr += 4;
    }

    return 1;
}

/*
 * Must be called only after NI6255Module::Init
 */
/*
int NI6723Module::InitDMA ()
{
	if (!bar0)
		return -1;
	
	bar0 = bus->createAddressSpace(kPCI_BAR0);
	mite = new tMITE(bar0);
	mite->setAddressOffset(0x500);
	dma = new tDMAChannel(bus, mite);
	
	return 1;
}
*/
/*
void NI6723Module::Reset() 
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
*/

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
int NI6723Module::Configure(int inputChannels, int outputChannels)
{
	if (!board)
		return -1;
	
	aiChannels = (inputChannels > AI_CHANNELS) ? AI_CHANNELS : inputChannels;
	aoChannels = (outputChannels > AO_CHANNELS) ? AO_CHANNELS : outputChannels;
	
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
/*
int NI6723Module::ConfigureDMA()
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
*/

/*
int NI6723Module::InitIRQ (int irq)
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
*/

/*
 * First start the DMA engine and then the  acquisition process (NI6255Module::Start())
 */
/*
int NI6723Module::StartDMA ()
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
*/
int NI6723Module::Start ()
{
	if (!board)
		return -1;
	
	// Single point ouput on AO channels
	board->AOMisc.writeAOEnable(1);
	board->AOMisc.writeAOUpdateMode(t6723::tAOMisc::kAOUpdateModeImmediate);
    return 1;
}
/*
void NI6723Module::AckIRQ ()
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

int NI6723Module::WaitIRQ ()
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
*/

/*
 * the method returns bytesAvailable in the DMA buffer 
 * userSizeinBytes is initilized in ConfigureDMA
 */
/*
int NI6723Module::PollDMA ()
{
	unsigned int bytesAvailable = 0;
	tDMAError status = kNoError;
	
	if (!dma)
		return -1;
	
    do 
    {
        //
        //  1. Use read to query the number of bytes in the DMA buffers
        //
    	status = dma->read(0, NULL, &bytesAvailable);
    }  while ( (bytesAvailable < AIuserSizeInBytes) && (kNoError == status));
    
    return bytesAvailable;
}

void NI6723Module::Poll()
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
*/


void NI6723Module::Write (u16 *buffer, int count)
{
	if (!board)
		return;
	int m;
	for (m = 0; m < aoChannels; m++)
		if (m < count) {
			(DirectDataArray[m])->write(buffer[m]>>3);
//			(DirectDataArray[m])->write(pippo);
		}
pippo *= -1;
}

void NI6723Module::Write (u32 *buffer, int count)
{
	if (!board)
		return;
	int m;
	for (m = 0; m < aoChannels; m++)
		if (m < count) {
			(DirectDataArray[m])->write(buffer[m]>>3);
//			(DirectDataArray[m])->write(pippo);
		}

pippo *= -1;
	// TODO eliminate the followinf line in RealTime
	memcpy (softCopyArray, buffer, sizeof(u32) * count);
}

void NI6723Module::Write (f32 *buffer, int count)
{
	if (!board)
		return;
	i32 value;
	int m;
	for (m = 0; m < aoChannels; m++)
		if (m < count) {
			/// convert from double - variables
			double corrected = buffer[m] * AOscale_gain[m] + AOscale_offset[m];
			double countsPerVolt = 8192.0 / 20.0;  // 13-bits over -10V..+10V
			double rounder;
			
			if ( corrected >= 0 )
				rounder = 0.5;
			else
				rounder = -0.5;
	
			value = (unsigned)(countsPerVolt * corrected + rounder);
//			DirectDataArray[m]->write (pippo);
			DirectDataArray[m]->write (value);
		}
pippo *= -1;
}

/*
void NI6723Module::StopDMA ()
{
	if (!dma)
		return;
	
	dma->stop ();
	board->AI_AO_Select.setAI_DMA_Select(0);
	board->AI_AO_Select.flush();
}
*/

void NI6723Module::Stop ()
{
	if (!board)
		return;
	
	board->AOMisc.writeAOEnable(0);
	
}

/*
 * must be called before NI6255Module::Release()
 */
/*
void NI6723Module::ReleaseDMA ()
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

void NI6723Module::ReleaseIRQ ()
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
*/

void NI6723Module::Release()
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

NI6723Module::~NI6723Module()
{
	//StopDMA();
	Stop();
	
	//ReleaseIRQ();
	Release();
}
