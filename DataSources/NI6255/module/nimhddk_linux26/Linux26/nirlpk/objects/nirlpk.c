/*
    nirlpk.c
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

#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
    #error This version of nirlpk has only been tested on 2.6 kernels.
#endif

/* base module includes */
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>

/* interrupt in user space include */
#include <linux/irq.h>
#include <asm/uaccess.h>
#include <linux/profile.h>
#include <linux/interrupt.h>
#include <linux/poll.h>

/* for PCI */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
	#include <linux/config.h>
#endif
#include <linux/pci.h>

/* character device */
#include <linux/cdev.h>

/* proc file support */
#include <linux/proc_fs.h>

/* ioctl user mode acess */
#include <asm/uaccess.h>
#include <asm/io.h>

#include "nirlpk.h"
#include "ioctl.h"

#ifndef CONFIG_PCI
    #error "This driver requires PCI support."
#endif

#ifdef MODULE_LICENSE
    MODULE_LICENSE("Copyright (c) 2006 National Instruments Corporation.  All Rights Reserved.");
#endif

MODULE_AUTHOR("National Instruments Corp.");

/* 
    Globals 
*/

struct file_operations nNIRLP_fops = 
{
    .open    = nNIRLP_open,
    .release = nNIRLP_release,
    .mmap    = nNIRLP_mmap,
    .ioctl   = nNIRLP_ioctl,
};

#define nNIRLP_kDeviceMinorCount 256

static unsigned int nNIRLP_deviceMajor = 0;
static dev_t nNIRLP_deviceNumber;

static LIST_HEAD(nNIRLP_pciDeviceList);
static DEFINE_SEMAPHORE(nNIRLP_lock, 1); //new version of DECLARE_MUTEX
//static DECLARE_MUTEX(nNIRLP_lock);

#define nNIRLP_kNIPCIVendorId 0x1093

static const struct pci_device_id nNIRLP_kNIPCIDeviceIDTable[ ] = 
{
    { PCI_DEVICE(nNIRLP_kNIPCIVendorId, PCI_ANY_ID) },
    { 0, },
};

MODULE_DEVICE_TABLE(pci, nNIRLP_kNIPCIDeviceIDTable);

static struct pci_driver nNIRLP_pciDriver = 
{
    .name        = nNIRLP_kDriverAlias,
    .id_table    = nNIRLP_kNIPCIDeviceIDTable,
    .probe       = nNIRLP_pciProbe,
    .remove      = nNIRLP_pciRemove,
    .suspend     = NULL,
    .suspend_late     = NULL,
    .resume_early     = NULL,
    .resume      = NULL,
    .shutdown = NULL,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
    .enable_wake = NULL,
#endif   
};

/* ============================================================================= */


int nNIRLP_pciProbe (struct pci_dev *pci_device, const struct pci_device_id *id)
{
    struct nNIRLP_tPCIDevice *device = NULL;
        
    nNIRLP_printDebug("nNIRLP_pciProbe() device found - id: 0x%04x\n", pci_device->device);

    device = nNIRLP_tPCIDevice_create (pci_device);
    if (device == NULL)
        return -ENOMEM;
    
    down(&nNIRLP_lock);
    list_add(&device->link, &nNIRLP_pciDeviceList);
    up(&nNIRLP_lock);

    return 0; 
}

void nNIRLP_pciRemove(struct pci_dev *pci_device)
{
    struct nNIRLP_tPCIDevice *device = NULL; 
    
    nNIRLP_printDebug("nNIRLP_pciRemove\n");
    
    device = (struct nNIRLP_tPCIDevice*) pci_get_drvdata(pci_device);
    
    down(&nNIRLP_lock);
    list_del(&device->link);
    up(&nNIRLP_lock);
    
    nNIRLP_tPCIDevice_destroy (device);
}

/* ============================================================================= */
/* 
    load/unload functions
*/

static int  __init nNIRLP_init(void); 
static void __exit nNIRLP_exit(void);

/*
  static int __init nNIRLP_init(void)
 
  Description:  driver entry point - Enumerate all NI-PCI cards on the
                bus, get dynamic major number, and register with the
                kernel.
*/

int nNIRLP_init(void)
{
    int status = 0;

    nNIRLP_printDebug("nNIRLP_init\n");
    nNIRLP_printDebug("timestamp - %s %s\n", __DATE__, __TIME__);
    nNIRLP_printDebug("PAGE_SIZE: %lu\n", PAGE_SIZE);

    /* get a device number range*/

    if (nNIRLP_deviceMajor) /* static major number */
    {
        nNIRLP_deviceNumber = MKDEV(nNIRLP_deviceMajor, 0);
        status = register_chrdev_region (nNIRLP_deviceNumber, nNIRLP_kDeviceMinorCount, nNIRLP_kDriverAlias); 
    }
    else                    /* dynamic major number */
    {
        status = alloc_chrdev_region (&nNIRLP_deviceNumber, 0, nNIRLP_kDeviceMinorCount, nNIRLP_kDriverAlias);
        nNIRLP_deviceMajor = MAJOR(nNIRLP_deviceNumber);
    }

    if (status >= 0)
    {
        nNIRLP_printDebug("registering pci driver (major %i)\n", nNIRLP_deviceMajor);
        status = pci_register_driver(&nNIRLP_pciDriver);
        
        if (status >= 0)
        {   
            status = nNIRLP_mkProcEntries();
            if (status >= 0)
            {
                nNIRLP_printDebug("driver initialized sucessfully\n");
                return 0;
            }
            pci_unregister_driver(&nNIRLP_pciDriver);
        }
        unregister_chrdev_region ( nNIRLP_deviceNumber, nNIRLP_kDeviceMinorCount);
    }
    nNIRLP_printCritical("driver initialization failed: %i\n", status);
    return status; 
}

/*
  static void __exit nNIRLP_exit (void)
   
  Description:  exit driver - unregister driver and free resources
                allocated in the driver entry routine
*/

void nNIRLP_exit(void) 
{ 
    nNIRLP_printDebug ("nNIRLP_exit\n");

    nNIRLP_rmProcEntries();
    pci_unregister_driver(&nNIRLP_pciDriver);
    unregister_chrdev_region ( nNIRLP_deviceNumber, nNIRLP_kDeviceMinorCount);
    
    nNIRLP_printDebug("driver unloaded.\n"); 
}

/* ============================================================================= */

/*
  Driver functions
*/

/*
  int nNIRLP_open (struct inode *inode, struct file *filp)
  
  Description: Opens a reference to the driver. No device is assigned
*/

int nNIRLP_open (struct inode *inode, struct file *filep)
{
    struct nNIRLP_tDriverContext *context = NULL; 

    nNIRLP_printDebug("nNIRLP_open(inode (%p), file (%p))\n", inode, filep);
    
    context = nNIRLP_tDriverContext_create(); 
    if ( context == NULL )
        return -ENOMEM;
    
    context->device = container_of(inode->i_cdev, struct nNIRLP_tPCIDevice, cdev);
    filep->f_op = &nNIRLP_fops;
    filep->private_data = (void *)context;

    nNIRLP_printDebug("minor %i\n", MINOR(context->device->cdev.dev));
    return 0;
}

/*
  void nNIRLP_release (struct inode *inode, struct file *filp)
 
  Description:  Close the file and clean up
*/

int nNIRLP_release (struct inode *inode, struct file *filep)
{
    struct nNIRLP_tDriverContext *context = (struct nNIRLP_tDriverContext *)filep->private_data;

    nNIRLP_printDebug("nNIRLP_release\n");

    nNIRLP_tDriverContext_destroy (context); 

    nNIRLP_printDebug ("exit release\n");
    return 0;
}

/*
  struct nNIRLP_vmops
  
  Description:  these are the vma functions for a buffer used for DMA.  
                It maps more memory into the user process each time it page faults.
*/

void nNIRLP_vmaOpen(struct vm_area_struct *vma)
{
    nNIRLP_printDebug("vmaOpen (0x%p)\n", vma);
}

void nNIRLP_vmaClose(struct vm_area_struct *vma)
{
    nNIRLP_printDebug("vmaClose (0x%p)\n", vma);
    nNIRLP_tDMA_fixPageCount ((struct nNIRLP_tDMA *)vma->vm_private_data, 0);
}

/* 
  nNIRLP_vmaNopage
    
  Description: virtual memory area (vma) page fault handler.
               (*nopage)() prototype changed in 2.6.1 
*/

struct page *nNIRLP_vmaNopage(struct vm_area_struct *vma, unsigned long address, int *type)
{
    struct nNIRLP_tDMA *dma = vma->vm_private_data;
    struct page *page;    

    if (type != NULL)
        *type = VM_FAULT_MINOR;

    address &= PAGE_MASK;

     nNIRLP_printDebug ("nopage:\n");
     nNIRLP_printDebug (" fault : 0x%p\n", (void*)address);
     nNIRLP_printDebug (" buffer: 0x%p\n", (dma->address.cpu + (address - vma->vm_start)));

    page =  virt_to_page( dma->address.cpu + (address - vma->vm_start) );

     nNIRLP_printDebug (" page count: %d\n", page_count(page));

    get_page (page);
    return page; 
}
/*
169  * vm_fault is filled by the the pagefault handler and passed to the vma's
170  * ->fault function. The vma's ->fault is responsible for returning a bitmask
171  * of VM_FAULT_xxx flags that give details about how the fault was handled.
172  *
173  * pgoff should be used in favour of virtual_address, if possible. If pgoff
174  * is used, one may set VM_CAN_NONLINEAR in the vma->vm_flags to get nonlinear
175  * mapping support.
176  */
/*
177 struct vm_fault {
178         unsigned int flags;              FAULT_FLAG_xxx flags
179         pgoff_t pgoff;                   Logical page offset based on vma
180         void __user *virtual_address;    Faulting virtual address
181 
182         struct page *page;               ->fault handlers should return a
183                                          page here, unless VM_FAULT_NOPAGE
184                                          is set (which is also implied by
185                                          VM_FAULT_ERROR).
186                                          
187 };*/

//int fault (struct vm_area_struct *vma, struct vm_fault *vmf);
struct page *nNIRLP_vmaFault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
    struct nNIRLP_tDMA *dma = vma->vm_private_data;
    struct page *page;
    unsigned long address;   

//    if (type != NULL)
//        *type = VM_FAULT_MINOR;

    address = (unsigned long)vmf->virtual_address;
    address &= PAGE_MASK;

     nNIRLP_printDebug ("nopage:\n");
     nNIRLP_printDebug (" fault : 0x%p\n", (void*)address);
     nNIRLP_printDebug (" buffer: 0x%p\n", (dma->address.cpu + (address - vma->vm_start)));

    page =  virt_to_page( dma->address.cpu + (address - vma->vm_start) );

     nNIRLP_printDebug (" page count: %d\n", page_count(page));

    get_page (page);
    vmf->page = page;
    return 0; 
}

static struct vm_operations_struct nNIRLP_vmops = 
    {   
        /*.open   = nNIRLP_vmaOpen,*/
        .close  = nNIRLP_vmaClose,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
        .nopage = nNIRLP_vmaNopage,
#else
        .fault = nNIRLP_vmaFault,
#endif
    };

/*
  int nNIRLP_mmap(struct file *filep, struct vm_area_struct *vma)
  
  Description: handle mmap system call
*/

int nNIRLP_mmap(struct file *filep, struct vm_area_struct *vma)
{
    unsigned long physicaladdr = 0;
    unsigned long vsize = vma->vm_end - vma->vm_start;
    int i;
    
    struct nNIRLP_tDriverContext *context = (struct nNIRLP_tDriverContext *)filep->private_data;
    
    int status = 0;
    int isValidBAR = 0;
    
    nNIRLP_printDebug("mmap: fuck\n");
    nNIRLP_printDebug("mmap: vma->vm_start = 0x%lx [phys @ %p]\n",vma->vm_start, (void*)virt_to_phys((void*)(vma->vm_start)));
    nNIRLP_printDebug("mmap: vma->vm_end   = 0x%lx\n",vma->vm_end);
    nNIRLP_printDebug("mmap: vma->vm_pgoff = 0x%lx\n",vma->vm_pgoff);
    
    physicaladdr = vma->vm_pgoff << PAGE_SHIFT; 
    
    /*
        Check if the physical address refers to a device resource
    */
    for (i = 0; i < nNIRLP_kPciMaxNumberOfBARs; ++i)
    {
        if (context->device->bar[i] == physicaladdr)
            isValidBAR = 1; 
    }
    
    if (isValidBAR)
    {
        vma->vm_flags |= (VM_IO | VM_RESERVED);

#ifdef nNIRLP_kRemapPfnRange
        if(remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
                                       vsize, vma->vm_page_prot))
#else
        if(remap_page_range(vma, vma->vm_start, vma->vm_pgoff << PAGE_SHIFT,
                                       vsize, vma->vm_page_prot))
#endif
        {
            nNIRLP_printDebug("could not map device address\n");
            return -EFAULT; // or -EAGAIN
        }
    }
    else /* DMA buffer */
    {
        struct nNIRLP_tDMA *dma; 
        
        status = -ENXIO;
        
        down(&context->lock);
        list_for_each_entry(dma, &context->dmalist, link)
        {
            if (dma->address.bus == physicaladdr)
            {
                status = 0; 
                break;
            }
        }
        up(&context->lock);
    
        if (status >= 0)
        {
            /* 
                Found a valid dma block for the address
                Let the page fault handler map the DMA buffer 
            */
            vma->vm_ops = &nNIRLP_vmops;
            vma->vm_private_data = dma;
        
            nNIRLP_printDebug("Found a valid block for the address %p\n", physicaladdr);
	    nNIRLP_printDebug("dma->address.bus %p dma->address.cpu %p\n", dma->address.bus, dma->address.cpu);
            nNIRLP_tDMA_fixPageCount (dma, 1);
        }
	else
            nNIRLP_printDebug("Not found a valid block for the address %p\n", physicaladdr);
    }

    if (status >= 0)
    {
        vma->vm_flags |= (VM_RESERVED | VM_LOCKED | VM_SHARED | VM_DONTCOPY | VM_DONTEXPAND);    
    }

    nNIRLP_printDebug("mmap: vma->vm_start = 0x%lx [phys @ %p]\n",vma->vm_start, (void*)virt_to_phys((void*)(vma->vm_start)));
    return status;
}

/*
  int nNIRLP_ioctl (...)
  
  Description:  handle iocontrol commands
*/

int nNIRLP_ioctl(struct inode *inode, struct file *filep, unsigned int cmd, unsigned long arg)
{
    struct nNIRLP_tDriverContext *context = (struct nNIRLP_tDriverContext *)filep->private_data;

    int status = 0; 

    /*
        Validate using ioctl command encoding
    */
    
    if (_IOC_TYPE(cmd) != NIRLP_IOCTL_MAGIC) 
        return -ENOTTY; /* posix */
    
    if (_IOC_NR(cmd) > NIRLP_IOCTL_MAX) 
        return -ENOTTY; /* posix */

    /*
        Validate user mode access - all commands pass a pointer to read or write data
    
        - If user mode is reading data, check that the kernel can write to user mode
        - If user mode is writing data, check that the kernel can read from user mode
    */
    
    if (_IOC_DIR(cmd) & _IOC_READ)
    {
        if( !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd)) )
            return -EFAULT;
    }
    
    if (_IOC_DIR(cmd) & _IOC_WRITE)
    {
        if( !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd)) )
            return -EFAULT;
    }
    
    switch (cmd) 
    {
        case NIRLP_IOCTL_ALLOCATE_DMA_BUFFER:
        {
            /*  
                arg:
                IN:  (unsigned long *) size of DMA buffer
                OUT: (unsigned long *) physical address if the allocated buffer
            */
            
            struct nNIRLP_tDMA *dma = NULL;
            unsigned long size = 0;
            
            status = __get_user(size, (unsigned long __user *)arg );
            if (status < 0)
                break; 
            
            dma = nNIRLP_tPCIDevice_allocateDMA (context->device, size);
            if (dma == NULL)
            {
                status = -ENOMEM;
                break; 
            }
            
            status = __put_user(dma->address.bus, (unsigned long __user *)arg);
            
            if (status >= 0)
            {
                down(&context->lock);
                list_add(&dma->link, &context->dmalist);
                up(&context->lock);
            }
            else
            {
                nNIRLP_tPCIDevice_freeDMA(context->device, dma);
            }
            
            break;
        }

        case NIRLP_IOCTL_GET_PHYSICAL_ADDRESS: /* device addresses only */
        {
            /*  
                arg:
                IN:  (unsigned long *) bar index
                OUT: (unsigned long *) physical address for the address space requested
            */
            
            unsigned long address = 0; 
            unsigned long requestedIdx = 0;
            
            status = __get_user(requestedIdx, (unsigned long __user *)arg );
            if (status < 0)
                break;
            
            if (requestedIdx < 0 || requestedIdx > nNIRLP_kPciMaxNumberOfBARs)
            {
                status = -EINVAL;
                break;
            }
            
            address = context->device->bar[requestedIdx];
            
            status = __put_user(address, (unsigned long __user *)arg);
            
            break;
        }

        case NIRLP_IOCTL_FREE_DMA_BUFFER:
        {
            /*  
                arg:
                IN:  (unsigned long *) DMA buffer physical address
            */
            
            struct nNIRLP_tDMA *cursor;
            struct nNIRLP_tDMA *next;
            unsigned long address = 0; 

            status = __get_user(address, (unsigned long __user *)arg );
            if (status < 0)
                break;
            
            down(&context->lock);
            list_for_each_entry_safe(cursor, next, &context->dmalist, link)
            {
                if (cursor->address.bus == address)
                {
                    nNIRLP_printDebug("found dma descriptor: 0x%p\n", cursor);
                    list_del(&cursor->link);
                    nNIRLP_tPCIDevice_freeDMA(context->device, cursor);
                    break;
                }
            }
            up(&context->lock);

            break;
        }

        default:
        {
            /* unknown command */
            nNIRLP_printDebug("ioctl: cmd 0x%x unknown\n", cmd);
            
            status = -ENOTTY;  /* posix */
            break;
        }
    }
    
    return status;
}

/* ============================================================================= */

/*
    nNIRLP_tPCIDevice functions
*/

int nNIRLP_getDeviceNumber(dev_t *devnumber)
{
    /*
       For now just increment the minor count until 
       the end of the allocated range. 
       If necessary implement a map to track free
       minor numbers and reuse them
   */
    static int minor = 0;
    int status = 0;

    down(&nNIRLP_lock);
    if (minor == nNIRLP_kDeviceMinorCount)
    {
        status = -ENODEV;
    }
    else
    {
    	*devnumber = MKDEV(nNIRLP_deviceMajor, minor);
    	++minor;
    }
    up(&nNIRLP_lock);

    return status;
}

/*
  nNIRLP_tPCIDevice_create (...)

  Description:  Create a new pci device object. Gets the device BARs and sets 
                the DMA mask
*/

struct nNIRLP_tPCIDevice *nNIRLP_tPCIDevice_create (struct pci_dev *pci_device)
{
    struct nNIRLP_tPCIDevice *device = NULL;
    int i; 
    dev_t devnumber;
    
    if (NULL == pci_device)
        return NULL; 
   
    if (nNIRLP_getDeviceNumber(&devnumber) < 0)
    {            
        nNIRLP_printCritical("Could not get a device number");
        return NULL; 
    }
    
    device = (struct nNIRLP_tPCIDevice *) kmalloc (sizeof (struct nNIRLP_tPCIDevice), GFP_KERNEL);
    if (NULL == device)
    {            
        nNIRLP_printCritical("Could not kmalloc space for device!");
        return NULL; 
    }
    
    nNIRLP_printDebug("Allocated tPCIDevice (%p)\n", device);
    
    /* initialize tPCIDevice object */

    INIT_LIST_HEAD(&device->link);
    
    for(i = 0; i < nNIRLP_kPciMaxNumberOfBARs; ++i)
    {
        u32 bar;

        if(pci_read_config_dword(pci_device, 0x10 + 0x04 * i, &bar))
        {
            nNIRLP_printCritical ("Could not read bar%d\n", i);
            kfree(device);
            return NULL;
        }

        nNIRLP_printDebug("  bar%i: 0x%08x\n", i, bar);
        device->bar[i] = bar; 
    }
    
    /*  initialize pci_dev object */     
    if (pci_set_dma_mask (pci_device, 0xffffffff) >= 0)
    {
        if (pci_enable_device(pci_device) >= 0)
        {
            pci_set_master(pci_device);
            pci_set_drvdata(pci_device, device);
            device->pci_device = pci_device;  

            /* initialize cdev object */ 
            cdev_init (&device->cdev, &nNIRLP_fops);
            device->cdev.owner = THIS_MODULE;
            
            if (kobject_set_name (&device->cdev.kobj, "nirlpk%d", MINOR(devnumber)) >= 0)
            {
                if (cdev_add (&device->cdev, devnumber, 1) >= 0)
                {
                    return device;
                }
                kobject_put(&device->cdev.kobj);
            }
            pci_disable_device(pci_device);
        }
    }
    kfree(device);
    return NULL;
}

/*
  nNIRLP_tPCIDevice_destroy (struct nNIRLP_tPCIDevice *device)

  Description:  Destroys a pci device object
*/

void nNIRLP_tPCIDevice_destroy (struct nNIRLP_tPCIDevice *device)
{
    if (NULL == device)
    {
        nNIRLP_printWarning ("tPCIDevice: attempted to destroy a NULL pointer\n");
        return;
    }
    
    pci_disable_device(device->pci_device);
    pci_set_drvdata(device->pci_device, NULL);
    
    device->pci_device = NULL;
    cdev_del(&device->cdev);
    kfree(device); 
    
    nNIRLP_printDebug("free tPCIDevice (%p)\n", device);
}

/*
  nNIRLP_tPCIDevice_allocateDMA (...)

  Description:  allocate a DMA buffer that the device can access. 
                returns a tDMA descriptor
*/

struct nNIRLP_tDMA *nNIRLP_tPCIDevice_allocateDMA (struct nNIRLP_tPCIDevice *device, size_t size)
{
    struct nNIRLP_tDMA *dma; 
    void       *cpu_addr; 
    dma_addr_t  bus_addr;

    if (NULL == device)
        return NULL; 

    /* allocate the dma descriptor */

    dma = (struct nNIRLP_tDMA *) kmalloc (sizeof (struct nNIRLP_tDMA), GFP_KERNEL);
    if (NULL == dma)
    {            
        nNIRLP_printCritical("Could not kmalloc space for dma descriptor\n");
        return NULL; 
    }

    nNIRLP_printDebug ("allocated tDMA (0x%p)\n", dma);

    cpu_addr = pci_alloc_consistent(device->pci_device, size, &bus_addr);
    if (NULL != cpu_addr)
    {
        INIT_LIST_HEAD(&dma->link);
        dma->address.bus   = bus_addr;
        dma->address.cpu   = cpu_addr;
        dma->size          = size;
  
        nNIRLP_printDebug ("allocated dma buffer %d bytes (descriptor: 0x%p)\n", size, dma);
        nNIRLP_printDebug ("  bus address 0x%p\n", (void *)bus_addr);
        nNIRLP_printDebug ("  cpu address 0x%p\n", cpu_addr);
    }
    else
    {   
        nNIRLP_printDebug("Could allocate DMA buffer, size %d\n", size);
        nNIRLP_printDebug ("free tDMA (0x%p)\n", dma);
        kfree(dma);
        dma = NULL;
    }

    return dma; 
}

/*
  nNIRLP_tPCIDevice_freeDMA (...)

  Description:  free a DMA buffer previously allocated with 
                nNIRLP_tPCIDevice_allocateDMA
*/

void nNIRLP_tPCIDevice_freeDMA 
    (struct nNIRLP_tPCIDevice *device, struct nNIRLP_tDMA *dma)
{
    if (NULL == device)
    {
        nNIRLP_printWarning("attempted to release dma using NULL device\n");
        return; 
    }
    
    if (NULL == dma)
    {
        nNIRLP_printWarning("attempted to release dma using NULL dma descriptor\n");
        return; 
    }

    /*
     * The code was used to esaminate the DMA buffer - can be deleted
     */
    /*
    int i, l, len; len = 0;
    char* DMABufferAddress = (char*)dma->address.cpu;
    char* outputBuffer = kmalloc(0x1000, GFP_KERNEL);
    memset (outputBuffer, 0, 0x1000);
    nNIRLP_printDebug("DMABufferAddress %p [phy @ %p]\n", DMABufferAddress, (void*)virt_to_phys((void*)DMABufferAddress));
    if (outputBuffer) {    
        if (DMABufferAddress) 
            for (i=0; i<32; i++) {
                for (l=0; l<32; l++)
                    len += sprintf((outputBuffer + len), "%02x ", (0x00FF & (int)DMABufferAddress[(i*32)+l]));
                nNIRLP_printDebug("%s\n", outputBuffer);
                len = 0;
           } 
    }
    kfree(outputBuffer);
    */

    pci_free_consistent(device->pci_device, dma->size, dma->address.cpu, dma->address.bus);
    
    nNIRLP_printDebug ("free dma buffer %d bytes (descriptor: 0x%p)\n", dma->size, dma);
    nNIRLP_printDebug ("  bus address 0x%p\n", (void *)dma->address.bus);
    nNIRLP_printDebug ("  cpu address 0x%p\n",         dma->address.cpu);
    
    kfree(dma); 
    nNIRLP_printDebug ("free tDMA (%p)\n", dma);
}

/* ============================================================================= */

/*
    nNIRLP_tDMA functions
*/

/*
  nNIRLP_tDMA_fixPageCount (...)

  Descrition:   fix the page_count of all the pages in the DMA buffer. 
                This operation is required to mmap multi-page (order > 0) buffers
                up -> 1 - fixes the page_count before mmap
                up -> 0 - reverts the page_count
*/

void nNIRLP_tDMA_fixPageCount (struct nNIRLP_tDMA *dma, int up)
{
    unsigned long address = (unsigned long)dma->address.cpu;
    unsigned long size    = dma->size; 
    unsigned long i;

    nNIRLP_printDebug ("fix page count %s \n", up ? "up":"down");

    for (i = address; i < address + size; i += PAGE_SIZE)
    {
        struct page *page; 
        page = virt_to_page (i);
        
        if (up)
        {   
            // SetPageReserved(page);
            get_page (page);
        }
        else
        {
           // ClearPageReserved(page);
           put_page_testzero (page); // or __put_page(page)
        }
        nNIRLP_printDebug("  i: 0x%lx, page count: %d\n", i, page_count(page)); 
    }
}

/* ============================================================================= */

/*
    nNIRLP_tDriverContext functions
*/

/*
  nNIRLP_tDriverContext_create ()

  Descrition:   Create a driver context object.  The context will be added to the
                file descriptor on open (filep->private_data)
                The driver context will keep track of the device and the dma
                buffers.
*/

struct nNIRLP_tDriverContext *nNIRLP_tDriverContext_create (void)
{
    struct nNIRLP_tDriverContext *context;

    context = (struct nNIRLP_tDriverContext *) 
        kmalloc (sizeof (struct nNIRLP_tDriverContext), GFP_KERNEL);

    if (NULL == context)
    {            
        nNIRLP_printCritical("Could not kmalloc space for driver context");
        return NULL; 
    }
    
    nNIRLP_printDebug("Allocated nNIRLP_tDriverContext (%p)\n", context);

    /* initialize tDriverContext */
    context->device  = NULL; 
    INIT_LIST_HEAD(&context->dmalist);
    sema_init(&context->lock, 1);

    return context;
}

/*
  nNIRLP_tDriverContext_destroy (...)

  Descrition:   Destroy the driver context (on release).  The dma 
                buffer list should be empty
*/

void nNIRLP_tDriverContext_destroy (struct nNIRLP_tDriverContext *context)
{
    struct nNIRLP_tDMA *cursor;
    struct nNIRLP_tDMA *next;
    
    if (NULL == context)
    {
        nNIRLP_printWarning ("tDriverContext: attempted to destroy a NULL context\n");
        return;
    }
    
    /* cleaup stale dma allocations */
    down(&context->lock);
    list_for_each_entry_safe(cursor, next, &context->dmalist, link)
    {
        list_del(&cursor->link);
        nNIRLP_tPCIDevice_freeDMA (context->device, cursor);
    }
    up(&context->lock);
    
    context->device = NULL;

    nNIRLP_printDebug("free nNIRLP_tDriverContext (%p)\n", context);
    kfree (context); 
}

/* ============================================================================= */

/*
    proc file support
*/

static struct proc_dir_entry *nNIRLP_procDir = NULL; 

/*
  void nNIRLP_procRead ()
 
  Description:  Called by the kernel when /proc/nirplk/nirlpk is read.  The 
                user process reads a text file with the driver's device list
                information. 
                This file is used by the osinterface
*/

int nNIRLP_procRead (char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
    int len   = 0;
    int index = 0; 
    struct nNIRLP_tPCIDevice *device;

    down(&nNIRLP_lock);
    
    list_for_each_entry(device, &nNIRLP_pciDeviceList, link)
    {
        len += sprintf (buf + len, "%i %u %u %d:%d\n", 
            index, 
            device->pci_device->bus->number, 
            PCI_SLOT(device->pci_device->devfn),
            MAJOR(device->cdev.dev),
            MINOR(device->cdev.dev));
        
        ++index;

        if (len >= PAGE_SIZE - 100) /* support only one page of data */
        {
            len += sprintf (buf + len, "-- no enough space to list all devices --\n");
        }
    }
    
    up(&nNIRLP_lock);
    
    *eof = 1;
    return len;
}

/*
  void nNIRLP_readLsdaq ()
 
  Description:  Called by the kernel when /proc/nirplk/lsdaq is read.  The 
                user process reads a text file with the driver's device list
                information. 
                Provides a user-friendly list of detected devices. 
*/

int nNIRLP_procLsdaq (char *buf, char **start, off_t offset, int count, int *eof, void *data)
{ 
    int len = 0;
    struct nNIRLP_tPCIDevice *device;

    len += sprintf (buf + len, "Devices detected by nirlpk:\n");
    len += sprintf (buf + len, "-------------------------------------\n");

    down(&nNIRLP_lock);
    
    list_for_each_entry(device, &nNIRLP_pciDeviceList, link)
    {   
        len += sprintf (buf + len, "0x%04x        PXI%u::%u::INSTR\n", 
            device->pci_device->device, 
            device->pci_device->bus->number, 
            PCI_SLOT(device->pci_device->devfn) );
        
        if (len >= PAGE_SIZE - 100) /* support only one page of data */
        {
            len += sprintf (buf + len, "-- no enough space to list all devices --\n");
        } 
    }

    up(&nNIRLP_lock);
    
    *eof = 1;
    return len;
}

struct irq_proc {
	unsigned long irq;
	wait_queue_head_t q;
	atomic_t count;
	atomic_t count_en_dis;
	char devname[TASK_COMM_LEN];
};

static irqreturn_t irq_proc_irq_handler(int irq, void *vidp)
{
	struct irq_proc *idp = (struct irq_proc *)vidp;
 
	BUG_ON(idp->irq != irq);
	disable_irq_nosync(irq);
	atomic_inc(&idp->count_en_dis);

	atomic_inc(&idp->count);
	wake_up(&idp->q);
	return IRQ_HANDLED;
}

/*
 * Signal to userspace an interrupt has occured.
 */
static ssize_t irq_proc_read(struct file *filp, char  __user *bufp, size_t len, loff_t *ppos)
{
	struct irq_proc *ip = (struct irq_proc *)filp->private_data;
//	irq_desc_t *idp = irq_desc + ip->irq;
	int pending;

	DEFINE_WAIT(wait);

	if (len < sizeof(int))
		return -EINVAL;

	pending = atomic_read(&ip->count);
	if (pending == 0) {
//		if (idp->status & IRQ_DISABLED) {
		if (atomic_read(&ip->count_en_dis) > 0) {
			enable_irq(ip->irq);
			atomic_dec(&ip->count_en_dis);
		}

		if (filp->f_flags & O_NONBLOCK)
			return -EWOULDBLOCK;
	}

	while (pending == 0) {
		prepare_to_wait(&ip->q, &wait, TASK_INTERRUPTIBLE);
	pending = atomic_read(&ip->count);
	if (pending == 0)
			schedule();
		finish_wait(&ip->q, &wait);
		if (signal_pending(current))
			return -ERESTARTSYS;
	}

	if (copy_to_user(bufp, &pending, sizeof pending))
		return -EFAULT;

	*ppos += sizeof pending;
	
	atomic_sub(pending, &ip->count);
	return sizeof pending;
}

static int irq_proc_open(struct inode *inop, struct file *filp)
{
	struct irq_proc *ip;
	struct proc_dir_entry *ent = PDE(inop);
	int error;

	ip = kmalloc(sizeof *ip, GFP_KERNEL);
	if (ip == NULL)
		return -ENOMEM;
	
	memset(ip, 0, sizeof(*ip));
	strcpy(ip->devname, current->comm);
	init_waitqueue_head(&ip->q);
	atomic_set(&ip->count, 0);
	atomic_set(&ip->count_en_dis, 0);
	ip->irq = (unsigned long)ent->data;
	
	error = request_irq(ip->irq, irq_proc_irq_handler, IRQF_DISABLED, ip->devname, ip);
	if (error < 0) {
		kfree(ip);
		return error;
	}
	filp->private_data = (void *)ip;

	return 0;
}

static int irq_proc_release(struct inode *inop, struct file *filp)
{
	struct irq_proc *ip = (struct irq_proc *)filp->private_data;
	(void)inop;
	free_irq(ip->irq, ip);
	filp->private_data = NULL;
	kfree(ip);
	return 0;
}

static unsigned int irq_proc_poll(struct file *filp, struct poll_table_struct *wait)
{
	struct irq_proc *ip = (struct irq_proc *)filp->private_data;
//	irq_desc_t *idp = irq_desc + ip->irq;

	if (atomic_read(&ip->count) > 0)
		return POLLIN | POLLRDNORM; /* readable */

	/* if interrupts disabled and we don't have one to process... */
//	if (idp->status & IRQ_DISABLED)
//		enable_irq(ip->irq);
		if (atomic_read(&ip->count_en_dis) > 0) {
			enable_irq(ip->irq);
			atomic_dec(&ip->count_en_dis);
		}

	poll_wait(filp, &ip->q, wait);

	if (atomic_read(&ip->count) > 0)
		return POLLIN | POLLRDNORM; /* readable */

	return 0;
}

static struct file_operations irq_proc_file_operations = {
	.read = irq_proc_read,
	.open = irq_proc_open,
	.release = irq_proc_release,
	.poll = irq_proc_poll,
};

static struct file_operations *saved_fops;

/*
  void nNIRLP_mkProcEntries ()
 
  Description:  Creates proc filesystem files. 
*/

int nNIRLP_mkProcEntries (void)
{
    char name_entry[32];
    struct nNIRLP_tPCIDevice *device;
    nNIRLP_procDir = proc_mkdir (nNIRLP_kDriverAlias, NULL);

    if (NULL == nNIRLP_procDir)
        return -ENOMEM; 

    if (create_proc_read_entry(nNIRLP_kDriverAlias, 0, nNIRLP_procDir, nNIRLP_procRead, NULL))
    {
        if (create_proc_read_entry("lsdaq", 0, nNIRLP_procDir, nNIRLP_procLsdaq, NULL))
        {
        	goto creating_waiters;
            return 0; /* success */
        }
        remove_proc_entry(nNIRLP_kDriverAlias, nNIRLP_procDir);
    }
    remove_proc_entry(nNIRLP_kDriverAlias, NULL);
    goto creating_error;

creating_waiters:
    // creting entries to wait for interrupts
    down(&nNIRLP_lock);	
    list_for_each_entry(device, &nNIRLP_pciDeviceList, link)
    {   
    	/* create /proc/nirlpk/1234 */
        sprintf (name_entry, "%d", device->pci_device->irq);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
    	if (!proc_create_data(name_entry, 0600, nNIRLP_procDir, &irq_proc_file_operations, (void *)(long)device->pci_device->irq))
    		goto creating_error;
#else
	{
	    	struct proc_dir_entry *pde;
	    	if (!(pde = create_proc_entry(name_entry, 0600, nNIRLP_procDir)))
    			goto creating_error;
		else {
			saved_fops = (struct file_operations *)pde->proc_fops;
			pde->proc_fops = &irq_proc_file_operations;
 			pde->data = (void *)(long)device->pci_device->irq;
		}
	}
#endif
    }
    up(&nNIRLP_lock);
    return 0;

creating_error:    
    nNIRLP_printDebug("error creating proc entries\n");
    return -ENOMEM;
}

/*
  void nNIRLP_rmProcEntries ()
 
  Description:  remove proc filesystem files. 
*/

void nNIRLP_rmProcEntries(void)
{
    char name_entry[32];
    struct nNIRLP_tPCIDevice *device;

    if (nNIRLP_procDir != NULL)
    {
        list_for_each_entry(device, &nNIRLP_pciDeviceList, link)
        {   
    	    /* create /proc/nirlpk/1234 */
            sprintf (name_entry, "%d", device->pci_device->irq);
            remove_proc_entry(name_entry, nNIRLP_procDir);
        }

        remove_proc_entry("lsdaq", nNIRLP_procDir);
        remove_proc_entry(nNIRLP_kDriverAlias, nNIRLP_procDir);
    }
    remove_proc_entry(nNIRLP_kDriverAlias, NULL);
}

/* ============================================================================= */

/* 
    exported symbols - In Linux 2.6 symbols must be explicitly exported 
*/

module_init(nNIRLP_init);
module_exit(nNIRLP_exit);

