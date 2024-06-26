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
#include "t6723.h"
// -I module/nimseries/Library/
#include "common.h"

/*
#include "tMITE.h"
#include "tDMAChannel.h"
*/

#define NI6723_EEPROM_SIZE 1024
#define NI6723_AI_CHANNELS 0
#define NI6723_AO_CHANNELS 32

// require the inclusion of initialize6723.o
extern void test(iBus *bus);


class NI6723Module {
private:
	iBus *bus;
	
	u32 physicalBar1;
	
	tAddressSpace  bar0;
	tAddressSpace  bar1;
	
	t6723 *board;
	//tMITE *mite;
	//tDMAChannel *dma;
	
	static const int kEepromSize;
    u8 eepromMemory[NI6723_EEPROM_SIZE];

    static const int AI_CHANNELS;
    static const int AO_CHANNELS;

	int aiChannels;
	int aoChannels;

int pippo;
	
	//tScalingCoefficients AIscale[NI6723_AI_CHANNELS];
	//tScalingCoefficients AOscale[NI6723_AO_CHANNELS];
	double AOscale_gain[NI6723_AO_CHANNELS];
	double AOscale_offset[NI6723_AO_CHANNELS];

	t6723::tDirectData_t * DirectDataArray[NI6723_AO_CHANNELS];
	
//	TODO AI->AO
//	unsigned int AIuserSizeInSamples;
//	unsigned int AIuserSizeInBytes;
//	unsigned int AIdmaSizeInSamples;
//	unsigned int AIdmaSizeInBytes;
//	unsigned short AIdmaUser[NI6723_AI_CHANNELS];
	
	int boardIrq;
	int boardIrqfd;
	
public:
	
	u32 softCopyArray[NI6723_AO_CHANNELS];
	
	NI6723Module();
	
	/*
	 * Example parameter: 
	 * address = "PXI11::15::INSTR"
	 * format => "PXI%d::%d::INSTR"
	 */
	int Init (char* address);
//	int InitDMA (); // added 23 july 2010 (DMA engine)
//	void Reset ();
	int Configure (int inputChannels, int outputChannels);
//	int ConfigureDMA (); // added 23 july 2010 (DMA engine)
//	int InitIRQ (int irq);
	
//	int StartDMA (); // added 23 july 2010 (DMA engine)
	int Start ();
private:
//	void AckIRQ ();
public:
//	int WaitIRQ ();
//	int PollDMA (); // added 23 july 2010 (DMA engine)
//	void Poll (); // modified 23 july 2010
	
	void Write (u16 *buffer, int count);
	void Write (u32 *buffer, int count);
	void Write (f32 *buffer, int count);
//	int WriteDMA(u32 *buffer, int count); 
//	int WriteDMA(f32 *buffer, int count); 

//	void StopDMA (); // added 23 july 2010 (DMA engine)
	void Stop ();
//	void ReleaseDMA (); // added 23 july 2010 (DMA engine)
//	void ReleaseIRQ ();
	void Release ();

	~NI6723Module();
};



#endif /*NI6255_DRIVER_H_*/
