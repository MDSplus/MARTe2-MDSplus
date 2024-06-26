/*
 * author: Antonio Barbalace
 * notes: based on aiex1.cpp aoex1.cpp from National Instruments
 * date: 04/05 may 2010
 */

#ifndef NI6255_DRIVER_H_
#define NI6255_DRIVER_H_


// -I module/nimhddk_linuxdevmem/
#define kLittleEndian 1
#define kGNU 1
#include "osiBus.h"

// -I module/nimseries/ChipObjects/
#include "tMSeries.h"
// -I module/nimseries/Library/
#include "common.h"
#include "scale.h"

#include "tMITE.h"
#include "tDMAChannel.h"


#define NI6255_EEPROM_SIZE 1024
#define NI6255_AI_CHANNELS 80
#define NI6255_AO_CHANNELS 2
#define DMA_AI_SAMPLES 32
#define DMA_AO_SAMPLES 32


class NI6255Module {
private:
	iBus * bus;
	
//Gabriele	void * physicalBar1;
	unsigned long physicalBar1;
	
	tAddressSpace  bar0;
	tAddressSpace  bar1;
	
	tMSeries *board;
	tMITE *mite;
	tDMAChannel *dma;
	
	static const int kEepromSize;
    u8 eepromMemory[NI6255_EEPROM_SIZE];

    static const int AI_CHANNELS;
    static const int AO_CHANNELS;

	int aiChannels;
	int aoChannels;
	
	tScalingCoefficients AIscale[NI6255_AI_CHANNELS];
	tScalingCoefficients AOscale[NI6255_AO_CHANNELS];
	
	unsigned int AIuserSizeInSamples;
	unsigned int AIuserSizeInBytes;
	unsigned int AIdmaSizeInSamples;
	unsigned int AIdmaSizeInBytes;
	unsigned short AIdmaUser[NI6255_AI_CHANNELS];

int bytesAvailable;

unsigned int statistic_noBytesAvailable;
unsigned int statistic_tooBytesAvailable;
unsigned int last_tooBytesAvailable;

	int boardIrq;
	int boardIrqfd;
	
public:
	NI6255Module();
	
	/*
	 * Example parameter: 
	 * address = "PXI49::14::INSTR"
	 * format => "PXI%d::%d::INSTR"
	 */
	int Init (char* address);
	int InitDMA (); // added 23 july 2010 (DMA engine)
	void Reset ();
	int Configure (int inputChannels, int outputChannels);
	int ConfigureDMA (); // added 23 july 2010 (DMA engine)
	int InitIRQ (int irq);
	
	int StartDMA (); // added 23 july 2010 (DMA engine)
	int Start ();
private:
	void AckIRQ ();
public:
	int WaitIRQ ();
	int PollDMA (); // added 23 july 2010 (DMA engine)
	void Poll (); // modified 23 july 2010
	
	int Read (u16 *buffer, int count);
	int Read (u32 *buffer, int count);
	int Read (f32 *buffer, int count);
	int ReadDMA(u32 *buffer, int count); // added 23 july 2010 (DMA engine)
	int ReadDMA(f32 *buffer, int count); // added 23 july 2010 (DMA engine)
	
	void Write (u16 *buffer, int count);
	void Write (u32 *buffer, int count);
	void Write (f32 *buffer, int count);

	void StopDMA (); // added 23 july 2010 (DMA engine)
	void Stop ();
	void ReleaseDMA (); // added 23 july 2010 (DMA engine)
	void ReleaseIRQ ();
	void Release ();

	void GetStatistics(unsigned int * no, unsigned int * too, unsigned int * last);

	~NI6255Module();
};



#endif /*NI6255_DRIVER_H_*/
