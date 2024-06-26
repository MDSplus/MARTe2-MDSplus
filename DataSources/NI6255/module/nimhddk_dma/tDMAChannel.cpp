/*
   tDMAChannel.cpp

   $DateTime: 2008/08/20 11:09:56 $
   
*/ 
/*
    (C) Copyright 2006
    National Instruments Corp.
    All Rights Reserved.
*/
#include "tDMAChannel.h"

#ifndef ___tMITE_h___
 #include "tMITE.h"
#endif 

#ifndef ___tDMABuffer_h___
 #include "tDMABuffer.h"
#endif

#ifndef ___tLinearDMABuffer_h___
 #include "tLinearDMABuffer.h"
#endif


// --------------------------------------------------------------------
// tDMAChannel
// --------------------------------------------------------------------

tDMAChannel::tDMAChannel(iBus *bus, tMITE *mite):
    _miteChannel (mite), 
    _bus (bus),
    
    _readIdx (0), 
    _writeIdx (0), 
	_lastwriteIdx (0),

	_realDAR(0),
	_lastDAR(0),

    _direction (kIn), 
    _drq (0), 
    _xferWidth (k16bit), 
    _mode (kNone),

    _state (kUnknown), 
    _buffer (NULL)
{
    reset();
}

tDMAChannel::~tDMAChannel()
{};
    

tDMAError tDMAChannel::start(void)
{
    if (_state == kStopped)
        _configure ();
    
    if (_state != kConfigured)
        return kWrongState;
    
    _miteChannel->ChannelOperation.writeStart (1);
    
    _state = kStarted;
    return kNoError;
}

tDMAError tDMAChannel::stop()
{
    _miteChannel->ChannelOperation.writeStop (1); 
    
    _state = kStopped;
    return kNoError;
}

tDMAError tDMAChannel::reset()
{
    if ( _state == kStarted )
        stop();
    
    _miteChannel->ChannelOperation.writeDmaReset(1);
    
    _miteChannel->ChannelOperation.writeRegister(0);
    _miteChannel->ChannelControl.writeRegister(0);
    
    _miteChannel->BaseCount.writeRegister (0);
    _miteChannel->TransferCount.writeRegister(0);
    
    _miteChannel->MemoryConfig.writeRegister(0);
    _miteChannel->DeviceConfig.writeRegister(0);

    
    _miteChannel->BaseAddress.writeRegister(0);
    _miteChannel->MemoryAddress.writeRegister(0);
    _miteChannel->DeviceAddress.writeRegister(0);
    
    
    if ( _buffer != NULL )
    {
        _buffer->free();
        delete _buffer;
        _buffer = NULL; 
    }
        
    _readIdx  = 0; 
    _writeIdx = 0;
	_lastwriteIdx = 0;
    
    _xferWidth = k16bit;

    _state     = kIdle; 
    return kNoError;
}

tDMAError tDMAChannel::config 
    (u32 requestSource, tDMAMode mode, tDMADirection direction, u32 size, tXferWidth xferWidth)
{
    if (_state != kIdle)
        return kWrongState;
    
    // 1. Store DMA Channel configuration
    
    _mode      = mode; 
    _direction = direction;
    _drq       = requestSource;
    _xferWidth = xferWidth;
    _size      = size; 
    
    // 2. Allocate DMA buffer
    
    if (_mode == kNormal || _mode == kRing)
    {
        _buffer = new tLinearDMABuffer ( _bus );
    }
    
    if (_buffer == NULL)
        return kSpaceNotAvailable; 
    
    if ( _buffer->allocate(_size) < kStatusSuccess )
    {
        delete _buffer; 
        _buffer = NULL; 
        return kSpaceNotAvailable;
    }
    
    // 3. Configure DMA Channel
    
    _configure ();
    
    _state = kConfigured;
    return kNoError;
}

tDMAError tDMAChannel::read (u32 requestedBytes, u8 *userBuffer, u32 *bytesLeft)
{
    if ( kIdle == _state || kUnknown == _state )
        return kWrongState;
    
    // 1. Get number of bytes available in the buffer and validate
    
    u32 bytesInBuffer = _getBytesInBuffer ();
    
    // 1a. if bytes transfered exceed the buffer size, DMA buffer overflowed
    
    if ( bytesInBuffer > _size )
        return kBufferOverflow;
    
    // 1b. If the user didn't pass a buffer, just return the bytes available
    
    if (requestedBytes == 0 || userBuffer == NULL)
    {
        *bytesLeft = bytesInBuffer; 
        return kNoError; 
    }
    
    // 1c. Check if there's enough data to satisfy user request
    
    if ( requestedBytes > bytesInBuffer )
    {
        return kDataNotAvailable;
    }
    
    // 2. Move Data from DMA Buffer
    
    _buffer->read (requestedBytes, userBuffer);

    // 3. Check for overflow again
    //    Note that the read index is not updated.  Check for an overflow while
    //    moving the data out of the DMA buffer.
    
    bytesInBuffer = _getBytesInBuffer ();
    
    if ( bytesInBuffer > _size )
        return kBufferOverflow;
    
    // 4. No overflow during data move.  Update read index and recalculate
    //    Number of bytes available to return to the user.
    
    _readIdx += requestedBytes; 

    bytesInBuffer = _getBytesInBuffer();
    
    *bytesLeft = bytesInBuffer;
    
    return kNoError; 
}

tDMAError tDMAChannel::readUnsafe (u32 requestedBytes, u8 *userBuffer)
{
/*    if ( kIdle == _state || kUnknown == _state )
        return kWrongState;
  */  
    // 1. Get number of bytes available in the buffer and validate
/*    
    u32 bytesInBuffer = _getBytesInBuffer ();
  */  
    // 1a. if bytes transfered exceed the buffer size, DMA buffer overflowed
/*    
    if ( bytesInBuffer > _size )
        return kBufferOverflow;
  */  
    // 1b. If the user didn't pass a buffer, just return the bytes available
/*    
    if (requestedBytes == 0 || userBuffer == NULL)
    {
        *bytesLeft = bytesInBuffer; 
        return kNoError; 
    }
  */  
    // 1c. Check if there's enough data to satisfy user request
/*    
    if ( requestedBytes > bytesInBuffer )
    {
        return kDataNotAvailable;
    }
  */  
    // 2. Move Data from DMA Buffer
    
    _buffer->read (requestedBytes, userBuffer);

    // 3. Check for overflow again
    //    Note that the read index is not updated.  Check for an overflow while
    //    moving the data out of the DMA buffer.
/*    
    bytesInBuffer = _getBytesInBuffer ();
    
    if ( bytesInBuffer > _size )
        return kBufferOverflow;
  */  
    // 4. No overflow during data move.  Update read index and recalculate
    //    Number of bytes available to return to the user.
    
    _readIdx += requestedBytes; 
/*
    bytesInBuffer = _getBytesInBuffer();
    
    *bytesLeft = bytesInBuffer;
  */
    return kNoError; 
}

tDMAError tDMAChannel::write (u32 requestedBytes, u8 *userBuffer, u32 *bytesLeft)
{
    if ( kIdle == _state || kUnknown == _state )
        return kWrongState;
    
    // 1. Get number of bytes available/free in the buffer and validate:
    
    u32 bytesInBuffer = _getBytesInBuffer ();
    u32 bytesFree     = _size - bytesInBuffer;
    
    // 1a. if bytes transfered exceed the buffer size, DMA buffer underflow
    //     this occurs when the read index passes the write index
    
    if ( bytesInBuffer > _size )
       return kBufferUnderflow;
    
    // 1b. If the user didn't pass a buffer, just return the bytes available
    
    if (requestedBytes == 0 || userBuffer == NULL)
    {
        *bytesLeft = bytesFree; 
        return kNoError; 
    }
    
    // 1c. Check if there's enough space to write user data
    
    if ( requestedBytes > bytesFree )
    {
        return kSpaceNotAvailable;
    }
    
    // 2. Move Data to DMA Buffer
    
    _buffer->write (requestedBytes, userBuffer);
    
    // 3. Check for overflow again
    //    Note that the write index is not updated.  Check for an underflow while
    //    moving the data into the DMA buffer.
    
    bytesInBuffer = _getBytesInBuffer();
    bytesFree     = _size - bytesInBuffer; 
    
    if ( bytesInBuffer > _size )
        return kBufferUnderflow;
    
    // 4. No underflow during data move.  Update write index and recalculate
    //    Number of bytes in the buffer to return to the user.
    
    _writeIdx += requestedBytes; 

    bytesInBuffer = _getBytesInBuffer();
    bytesFree     = _size - bytesInBuffer; 
    
    *bytesLeft = bytesFree;
    
    return kNoError; 
}

void* tDMAChannel::getAddress(void)
{
    return ((tLinearDMABuffer*)_buffer)->_memory->getAddress();
}

void tDMAChannel::_configure ()
{
    
    _miteChannel->ChannelControl.setBurstEnable (1);
    _miteChannel->ChannelControl.setDir ( _direction == kIn ? 1 : 0 );
    
    _miteChannel->MemoryConfig.setRegister (0x00E00400);
    _miteChannel->MemoryConfig.setPortSize ( _xferWidth );
    
    _miteChannel->DeviceConfig.setRegister (0x00000440);
    _miteChannel->DeviceConfig.setPortSize ( _xferWidth );
    _miteChannel->DeviceConfig.setReqSource ( 4 + _drq );
    
    if ( _mode == kNormal )
    {
        u32 startAddress = _buffer->getStartAddressForMite ();
        
        _miteChannel->ChannelControl.setXMode (0);
        
        _miteChannel->MemoryAddress.writeRegister ( startAddress );
        _miteChannel->TransferCount.writeRegister ( _size );
    
        _miteChannel->BaseAddress.writeRegister ( startAddress );
        _miteChannel->BaseCount.writeRegister   ( _size );
    }
    else if ( _mode == kRing )
    {
        u32 startAddress = _buffer->getStartAddressForMite ();
        
        _miteChannel->ChannelControl.setXMode (2);
    
        _miteChannel->MemoryAddress.writeRegister ( startAddress );
        _miteChannel->TransferCount.writeRegister ( _size );
    
        _miteChannel->BaseAddress.writeRegister ( startAddress );
        _miteChannel->BaseCount.writeRegister   ( _size );
    }
    
    _miteChannel->ChannelControl.flush ();
    _miteChannel->MemoryConfig.flush ();
    _miteChannel->DeviceConfig.flush ();
    
    _miteChannel->DeviceAddress.writeRegister (0);
    
    _buffer->setLocation (0);
    _readIdx  = 0; 
    _writeIdx = 0;
    
    _state = kConfigured; 
    return;
}

u32 tDMAChannel::_getBytesInBuffer ()
{
    // 1. Update MITE's location
    
    if (_direction == kIn)
    {
		//
        // the device address indicates the number of bytes that have
        // been transfered from the device to the DMA FIFO.
        // Samples in the FIFO have not been transfered to host memory
        // so correct the number device address with the FIFO count.
        //
        // The loop avoids a _writeIdx underflows.
        //
        // 'tries' is there to have an upper bound, but it should never 
        // exceed 100.
        //
        
        u32 deviceAddress = 0;
		u32 deviceAddress1 = 0;
		u32 fifoCount = 0;
		u32 tries = 0;

		tBoolean stableDAR;
        
        do
        {
            deviceAddress  = _miteChannel->DeviceAddress.readRegister ();
            fifoCount      = _miteChannel->FifoCount.readFifoCR();
			deviceAddress1  = _miteChannel->DeviceAddress.readRegister ();

			stableDAR = (deviceAddress == deviceAddress1);
        
            ++tries; 
        } while ( !(stableDAR) && (tries < 100) );

		// If we made it out of this loop without a stable DAR then
		// the upcoming code is not likely to execute correctly.
		if (tries == 100)
		{
			// may want to throw an error or otherwise log this condition
			return 0;
		}

		// Update and store 64-bit DAR values.  This allows us to use 64-bit _writeIdx
		// and not worry about rollover

		if(_lastDAR > deviceAddress1)
		{ 
			_realDAR = (_realDAR & nOSINT100_mU64bitLiteral(0xFFFFFFFF00000000)) | deviceAddress1;
			_realDAR +=            nOSINT100_mU64bitLiteral(0x0000000100000000); //u32 maxint + 1
			_lastDAR = deviceAddress1;
		}
		else
		{
			_realDAR = (_realDAR & nOSINT100_mU64bitLiteral(0xFFFFFFFF00000000)) | deviceAddress1;
			_lastDAR = deviceAddress1;
		}

		// Update _writeIdx value.  _realDAR will [practically] never roll
		// over, so it is safe to simply subtract fifoCount.
		_writeIdx = _realDAR - fifoCount;

		// Check if _writeIdx went down from last time.
		// If so, is means that we're headed for an erroneous
		// kDataNotAvailable.  Reset _writeIdx to previous
		// value if necessary.  If not, update _lastwriteIdx

		if( ( _writeIdx < _lastwriteIdx ) || 
			( (_lastwriteIdx == 0) && (deviceAddress1 < fifoCount) ) )
			_writeIdx = _lastwriteIdx;
		else
			_lastwriteIdx = _writeIdx;
	}

    else
    {
        _readIdx = _miteChannel->DeviceAddress.readRegister ();
    }
    
    // 2. Calculate difference between read and write indexes
    //    checking for rollovers
    
    u32 bytesInBuffer = 0; 
    
    if ( _writeIdx < _readIdx )
    {
        bytesInBuffer = 0xffffffff - (u32)( _readIdx - _writeIdx );
        ++bytesInBuffer;
    }
    else
    {
        bytesInBuffer = (u32)(_writeIdx - _readIdx);   
    }    
    
    return bytesInBuffer;
}