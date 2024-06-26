/*
   tDMABuffer.h
    
   $DateTime: 2008/08/20 11:09:56 $
   
*/
/*
    (C) Copyright 2006
    National Instruments Corp.
    All Rights Reserved.
*/
#ifndef ___tDMABuffer_h___
#define ___tDMABuffer_h___

#include "osiTypes.h"

enum
{
    kErrorMemoryAllocation = -1
};

class tDMABuffer
{
    public:

        tDMABuffer();
        virtual ~tDMABuffer();
    
        inline u64  getLocation ();
        inline void setLocation (u64 location);
        
        virtual tStatus allocate (u32 size) = 0;
        virtual void free () = 0;
    
        virtual void read  (u32 requestedBytes, void *buffer) = 0;
        virtual void write (u32 requestedBytes, void *buffer) = 0;
    
        virtual u32 getStartAddressForMite () = 0;
    
    private:
        u64 _index;
};

inline u64 tDMABuffer::getLocation ()
{
    return _index; 
}

inline void tDMABuffer::setLocation (u64  location)
{
    _index = location; 
}

#endif // ___tDMABuffer_h___
