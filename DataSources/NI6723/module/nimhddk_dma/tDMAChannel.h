/*
   tDMAChannel.h

   DMA Channel controller

   $DateTime: 2008/08/20 11:09:56 $
   
*/
/*
    (C) Copyright 2006
    National Instruments Corp.
    All Rights Reserved.
*/

#ifndef ___tDMAChannel_h___
#define ___tDMAChannel_h___

#ifndef ___osiBus_h___
    #include "osiBus.h"
#endif 

/*!
    \brief DMA Errors
*/
typedef enum
{
    kNoError = 0,
    
    kBufferOverflow,
    kBufferUnderflow,
    kDataNotAvailable,
    kSpaceNotAvailable,
    kWrongState,
    kInvalidChannel,
    kInvalidInput

} tDMAError;

//forward class declarations

class tMITE;
class tDMABuffer; 
    
//==============================================================================
// tDMAChannel
//==============================================================================

/*!
    \brief DMA Channel controller

*/

class tDMAChannel
{
    
    public:
    
    /*!
        \brief DMA Transfer size
    
    */
    typedef enum 
    {
        k8bit  = 1,
        k16bit = 2,
        k32bit = 3
    } tXferWidth;
	
    /*!
        \brief DMA mode
    
    */
    typedef enum 
    {
        kNormal = 0,
        kRing,
        kLinkChain,
        kLinkChainRing,
        kNone,
    } tDMAMode;

    /*!
        \brief DMA direction
    
    */
    typedef enum 
    {
        kIn  = 0,
        kOut = 1
    } tDMADirection;

    /*!
        constructor
        
        Transition to \a Idle state
    */
    tDMAChannel  (iBus *bus, tMITE *mite);
    ~tDMAChannel (void);
    
    /*!
        \brief Configure DMA channel
        \param requestSource Select the hardware DMA Request Line
        \param mode DMA transfer Mode (Normal, Ring, Linked, ...)
        \param direction DMA transfer direction - kIn (Device to Memory), kOut (Memory to Device)
        \param size 
        \param xferWitdh
    
        Transition DMA Channel state to \a Configured
    */
    tDMAError config 
        (u32 requestSource, tDMAMode mode, tDMADirection direction, u32 size, tXferWidth xferWidth);
    
    /*!
        \brief Start DMA Transfer
        
        Transition to \a Running state
    */
    tDMAError start        (void);
    
    /*!
        \brief Stop DMA Transfer
        
        Transition to \a Configure state
    */
    tDMAError stop         (void);
    
    /*!
        \brief Read data from DMA Channel
        \param [in] requestedBytes Requested number of bytes to be read
        \param [out] buffer Buffer to copy the data
        \param [out] bytesLeft bytes left in the DMA buffer
    
        Can only be called in the \a Running state
    */
    tDMAError read         (u32 requestedBytes, u8 *buffer, u32 *bytesLeft);
    tDMAError readUnsafe   (u32 requestedBytes, u8 *buffer);

    tDMAError write        (u32 requestedBytes, u8 *buffer, u32 *bytesLeft);
    
    /*!
        \brief Stop DMA Transfer
        
        Transition to \a Idle state
    */
    tDMAError reset        (void);
    void* getAddress(void);

    private:
        
        void _configure ();
        u32 _getBytesInBuffer();
    
        typedef enum
        {
            kUnknown, 
            kIdle, 
            kConfigured, 
            kStarted,
            kStopped
        } tDMAState;
        
        
        tMITE         *_miteChannel; 
        iBus          *_bus;         
        
        u64           _readIdx;
        u64           _writeIdx;
		u64           _lastwriteIdx; //last "good" _writdeIdx snapshot

		u64			  _realDAR; //64-bit "real" DAR value
		u32			  _lastDAR; //DAR value from previous snapshot
    
        tDMADirection _direction;        
        u32           _drq;       
        tXferWidth    _xferWidth;
        tDMAMode      _mode;
        u32           _size;
        
        tDMAState     _state;
        tDMABuffer   *_buffer; 
    
    private:
        
        // Usage Guidelines
        tDMAChannel(const tDMAChannel& rhs);
        tDMAChannel& operator=(const tDMAChannel& rhs);
};

#endif // ___tDMAChannel_h___

