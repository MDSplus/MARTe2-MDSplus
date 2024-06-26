/*
*  osiUserCode.cpp (for Linux 2.6)
*
*  osiUserCode.cpp holds the two user defined functions
*  needed to port iBus to a target platform.
*
*  iBus* acquireBoard(char*brdLocation) -- constructs and initializes the iBus
*
*  void  releaseBoard(iBus *&bus) -- deletes and cleans up the iBus
*
*  $DateTime: 2006/10/24 23:40:45 $
*
*/

//Platform independent headers
#include "osiBus.h"


// Platform specific headers

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "nirlpk/ioctl.h"


struct tLinuxSpecific
{
    int fileDescriptor; 
    void *mappedMemory0;
    void *mappedMemory1;
};


static i32 getBoardId (u32 bus, u32 device);

iBus* acquireBoard(tChar* brdLocation)
{
    iBus *bus;

    unsigned long physicalBar0;
    unsigned long physicalBar1;

    void *mem0 = NULL;
    void *mem1 = NULL;
    
    u32 busNumber = 0; 
    u32 devNumber = 0; 
    i32 brdId; 
   
    sscanf (brdLocation,"PXI%d::%d::INSTR", &busNumber, &devNumber);

    brdId = getBoardId (busNumber, devNumber);

    if (brdId < 0)
    {   
        printf ("invalid device descriptor: %s\n", brdLocation);
        return NULL; 
    }

    int fd = 0;
    char file[32];
    
    sprintf(file, "/dev/nirlpk%d", brdId);
    
    fd = open (file, O_RDWR);
    if (fd <= 0 )
    {
        printf ("fail to open %s\n", file);
        return NULL;
    }

    physicalBar0 = 0; 
    if ( ioctl (fd, NIRLP_IOCTL_GET_PHYSICAL_ADDRESS, &physicalBar0) < 0 )
    {
       close(fd);
       return NULL;
    }
    
    physicalBar1 = 1; 
    if ( ioctl (fd, NIRLP_IOCTL_GET_PHYSICAL_ADDRESS, &physicalBar1) < 0 )
    {
       close(fd);
       return NULL;
    }

    mem0 = mmap (NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, fd, physicalBar0);
    mem1 = mmap (NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, fd, physicalBar1);

    if ( MAP_FAILED == mem0 || MAP_FAILED == mem1)
    {
        close (fd);
        return NULL;
    }

    // create a new iBus
    bus = new iBus(0, 0, mem0, mem1);
    bus->_physBar[0] = (u32)physicalBar0;
    bus->_physBar[1] = (u32)physicalBar1;
    bus->_physBar[2] = (u32)NULL;
    bus->_physBar[3] = (u32)NULL;
    bus->_physBar[4] = (u32)NULL;
    bus->_physBar[5] = (u32)NULL;

    tLinuxSpecific *specific =  new tLinuxSpecific;
    specific->fileDescriptor = fd;
    specific->mappedMemory0 = mem0;
    specific->mappedMemory1 = mem1;

    bus->_osSpecific = reinterpret_cast<void*> (specific);
    return bus;
}

void releaseBoard(iBus *&bus)
{
    tLinuxSpecific *specific = reinterpret_cast <tLinuxSpecific *> ( bus->_osSpecific );
   
    munmap (specific->mappedMemory0, 4096);
    munmap (specific->mappedMemory1, 4096);

    close (specific->fileDescriptor);

    delete specific;
    delete bus;
}

i32 getBoardId (u32 bus, u32 dev)
{
    FILE *fp; 
    i32 minor = -1;
   
    fp = fopen("/proc/nirlpk/nirlpk", "r"); 
    if (NULL == fp)
        return minor; 

    u32 _bus;
    u32 _dev; 
    i32 _major;
    i32 _minor;
    
    while (EOF != fscanf(fp, "%*d %d %d %d:%d\n", &_bus, &_dev, &_major, &_minor))
    {
        if ((bus == _bus) && (dev == _dev))
        {
            minor = _minor;
            break;
        }
    }
    
    fclose (fp); 
    return minor;
}

/*
   DMA Memory support
*/

class tLinuxDMAMemory : public tDMAMemory
{
    
    public:

    tLinuxDMAMemory (void * vAddress, u32 pAddress, u32 size, int fd):
        tDMAMemory (vAddress, pAddress, size),
        _fd(fd)
    {};
        
    ~tLinuxDMAMemory();
    
    private:
    
    int _fd;  
};


tDMAMemory * iBus::allocDMA (u32 size)
{      
    tLinuxSpecific *specific = reinterpret_cast <tLinuxSpecific*> ( _osSpecific );

    int fd = specific->fileDescriptor;
    unsigned long physicalAddress; 
    void *virtualAddress; 

    physicalAddress = (unsigned long) size;
    if ( ioctl (fd, NIRLP_IOCTL_ALLOCATE_DMA_BUFFER, &physicalAddress) < 0 )
    {
        return NULL;
    }

    virtualAddress  = mmap (NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, physicalAddress);
    if (virtualAddress == MAP_FAILED)
    {
        ioctl (fd, NIRLP_IOCTL_FREE_DMA_BUFFER, &physicalAddress);
        return NULL;
    }
    //printf("physicalAddress %p virtualAddress %p\n", physicalAddress, virtualAddress);

    tDMAMemory *dma = new tLinuxDMAMemory (virtualAddress, physicalAddress, size, fd);
    if (NULL == dma)
    {
        munmap (virtualAddress,  size);
        ioctl (fd, NIRLP_IOCTL_FREE_DMA_BUFFER, &physicalAddress );
    }

    return dma;
}

void iBus::freeDMA (tDMAMemory *mem)
{
    delete mem;
}

//
//  tLinuxDMAMemory
//

tLinuxDMAMemory::~tLinuxDMAMemory()
{
    unsigned long physicalAddress; 
   
    munmap (getAddress(),  getSize() );
   
    physicalAddress = getPhysicalAddress();
    ioctl (_fd, NIRLP_IOCTL_FREE_DMA_BUFFER, &physicalAddress );
    _fd = 0; 
}


