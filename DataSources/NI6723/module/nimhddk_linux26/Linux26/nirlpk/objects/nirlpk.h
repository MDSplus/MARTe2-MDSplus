/*
    nirlpk.h
    Linux 2.6 kernel driver for register-level programming
    
    $DateTime: 2006/10/24 23:40:45 $
*/
/*
    Copyright (c) 2006, National Instruments Corporation
    All rights reserved.
    
    Redistribution and use in source and binary forms, with or without modification, 
    are permitted provided that the following conditions are met:
    
    1. Redistributions of source code must retain the above copyright notice, 
       this list of conditions and the following disclaimer. 
    2. Redistributions in binary form must reproduce the above copyright notice, 
       this list of conditions and the following disclaimer in the documentation 
       and/or other materials provided with the distribution. 
    3. Neither the name of National Instruments Corporation nor the names of its 
       contributors may be used to endorse or promote products derived from this 
       software without specific prior written permission. 
    
    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
    POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef ___nirlpk_h___
#define ___nirlpk_h___

/*
    misc. definitions
*/

#define nNIRLP_kPciMaxNumberOfBARs  2
#define nNIRLP_kDriverAlias         "nirlpk"

/* ------------------------------------------------------------------------------------------ */

/* 
    trace/debug macros 
*/

#define nNIRLP_kEnableDebugPrints 1 

#if nNIRLP_kEnableDebugPrints
    #define nNIRLP_printDebug(fmt, args...) printk( KERN_DEBUG "nirlpk: " fmt, ## args)
#else
    #define nNIRLP_printDebug(fmt, args...)
#endif

#define nNIRLP_printCritical(fmt, args...) printk( KERN_CRIT "nirlp: CRITICAL ERROR: " fmt, ## args)
#define nNIRLP_printWarning(fmt, args...) printk( KERN_WARNING "nirlp: warning: " fmt, ## args)

/* ------------------------------------------------------------------------------------------ */

/* 
    file operations
*/

int nNIRLP_open    (struct inode *, struct file *);
int nNIRLP_release (struct inode *, struct file *);
//int nNIRLP_ioctl   (struct inode *, struct file *, unsigned int, unsigned long);
int nNIRLP_ioctl   (struct file *, unsigned int, unsigned long);
int nNIRLP_mmap    (struct file *, struct vm_area_struct *);

/* ------------------------------------------------------------------------------------------ */

/*
    nNIRLP_tDMA

    Holds information on a DMA buffer.  Use tPCIDevice factory functions to create/destroy
    DMA buffers.  
    
    The tDMA descriptor holds two addesses: cpu and bus:
    - cpu is the virtual address usable by the kernel to access the buffer.
    - bus is the address passed to the device.  Note that this address may not be the 
      physical address as seen by the CPU.  If the system has an IOMMU, the kernel may
      map a 32-bit bus address to a 64-bit physical CPU address. 
    
      If you need the physical cpu address use __pa(address.cpu)
*/

struct nNIRLP_tDMA
{
    struct list_head link;
    size_t size;
    struct 
    {
        void       *cpu; 
        dma_addr_t  bus;
    } address;
};

void nNIRLP_tDMA_fixPageCount (struct nNIRLP_tDMA *, int); 

/* ------------------------------------------------------------------------------------------ */

/*
    nNIRLP_tDriverContext

    Holds the driver state (context) for an opened session.
*/

struct nNIRLP_tDriverContext 
{
    struct semaphore          lock;
    struct list_head          dmalist;
    struct nNIRLP_tPCIDevice *device;
};

/* constructor/destructor */

struct nNIRLP_tDriverContext *nNIRLP_tDriverContext_create (void);
void nNIRLP_tDriverContext_destroy (struct nNIRLP_tDriverContext *);

/* ------------------------------------------------------------------------------------------ */

/*
    nNIRLP_tPCIDevice
*/

struct nNIRLP_tPCIDevice 
{   
    struct list_head link;
    struct cdev cdev; 
    struct device *device;
    struct pci_dev *pci_device;          /* linux pci device object */
    u32  bar[nNIRLP_kPciMaxNumberOfBARs]; /* pci device resources */
};

/* constructor/destructor */

struct nNIRLP_tPCIDevice *nNIRLP_tPCIDevice_create (struct pci_dev *);
void nNIRLP_tPCIDevice_destroy (struct nNIRLP_tPCIDevice *);

struct nNIRLP_tDMA *nNIRLP_tPCIDevice_allocateDMA (struct nNIRLP_tPCIDevice *, size_t);
void nNIRLP_tPCIDevice_freeDMA (struct nNIRLP_tPCIDevice *, struct nNIRLP_tDMA *);

/*
   pci device detection callbacks - registered using pci_register_driver
*/

int nNIRLP_pciProbe (struct pci_dev *, const struct pci_device_id *);
void nNIRLP_pciRemove (struct pci_dev *);

/* ------------------------------------------------------------------------------------------ */

/*
    proc file support
*/

int nNIRLP_mkProcEntries(void);
void nNIRLP_rmProcEntries(void);

#endif /* ___nirlpk_h___ */
