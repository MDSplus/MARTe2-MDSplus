/*
   tLinearDMABuffer.h
   
   $DateTime: 2008/08/20 11:09:56 $
   
*/
/*
    (C) Copyright 2006
    National Instruments Corp.
    All Rights Reserved.
*/
#ifndef ___tLinearDMABuffer_h___
#define ___tLinearDMABuffer_h___

#ifndef ___tDMABuffer_h___
 #include "tDMABuffer.h"
#endif

class iBus; 
class tDMAMemory; 

class tLinearDMABuffer : public tDMABuffer
{
    public:
        tLinearDMABuffer (iBus *bus);
        virtual ~tLinearDMABuffer ();
        
        virtual tStatus allocate (u32 size);
        virtual void free ();
    
        virtual void read  (u32 requestedBytes, void *buffer);
        virtual void write (u32 requestedBytes, void *buffer);
        
        virtual u32 getStartAddressForMite ();
    private:
        
        iBus       *_bus; 
        u64         _size; 

    public:
        tDMAMemory *_memory;
};

#endif // ___tLinearDMABuffer_h___
