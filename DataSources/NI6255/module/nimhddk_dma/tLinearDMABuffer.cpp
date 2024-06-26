/*
   tLinearDMABuffer.cpp

   $DateTime: 2008/08/20 11:09:56 $
   
*/
/*
    (C) Copyright 2006
    National Instruments Corp.
    All Rights Reserved.
*/
#include "tLinearDMABuffer.h"
#include "osiBus.h"

#include <string.h>
#include <stdio.h>

tLinearDMABuffer::tLinearDMABuffer (iBus *bus):
    tDMABuffer (),
    _bus (bus),
    _memory (NULL),
    _size(0)
{
}
    
tLinearDMABuffer::~tLinearDMABuffer ()
{
    free();
}

tStatus tLinearDMABuffer::allocate (u32 size)
{
    
    _memory = _bus->allocDMA (size);
    printf("_bus->allocDMA (%d) = %p (virt) %p (phys)\n", size, _memory->getAddress(), _memory->getPhysicalAddress());
    
    if ( NULL == _memory )
        return kErrorMemoryAllocation;     
    
    _size = size;
    return kStatusSuccess; 
}

void  tLinearDMABuffer::free ()
{
    if (NULL != _memory)
    {
        _bus->freeDMA (_memory);
        _memory = NULL;
    }
}

void tLinearDMABuffer::read (u32 requestedBytes, void *buffer)
{
    u64 current = getLocation ();
    u64 end     = (current + requestedBytes) % _size; 
    
    u8 *address = (u8 *) _memory->getAddress();
    
    u64 offset = 0; 
    
    if ( end <= current )
    {
        offset = _size - current;
        memcpy ( buffer, address + current , offset);
        
        buffer = (void*) ((u8 *) buffer + offset); 
        current = 0;
    }
    
    memcpy( buffer, address + current , (end - current) );
    
    setLocation (end);
}


void tLinearDMABuffer::write (u32 requestedBytes, void *buffer)
{
    u64 current = getLocation ();
    u64 end     = (current + requestedBytes) % _size; 
    
    u8 *address = (u8 *) _memory->getAddress();
    
    u64 offset = 0; 
    
    if ( end <= current )
    {
        offset = _size - current;
        memcpy (address + current , buffer, offset);
        
        buffer = (void*) ((u8 *) buffer + offset); 
        current = 0;
    }
    
    memcpy(address + current, buffer, (end - current) );
    
    setLocation (end);
}

u32 tLinearDMABuffer::getStartAddressForMite ()
{
    return _memory->getPhysicalAddress();
}
