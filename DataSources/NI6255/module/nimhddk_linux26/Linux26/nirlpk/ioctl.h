/*
    nirlpk/ioctl.c
        io control codes for nirlpk

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
#ifndef ___nirlpk_ioctl_h___
#define ___nirlpk_ioctl_h___

#include <linux/ioctl.h>

#define NIRLP_IOCTL_MAGIC   0xbb

#define NIRLP_IOCTL_GET_PHYSICAL_ADDRESS _IOWR(NIRLP_IOCTL_MAGIC, 1, unsigned long) 
#define NIRLP_IOCTL_ALLOCATE_DMA_BUFFER  _IOWR(NIRLP_IOCTL_MAGIC, 2, unsigned long)
#define NIRLP_IOCTL_FREE_DMA_BUFFER      _IOW(NIRLP_IOCTL_MAGIC, 3, unsigned long)

#define NIRLP_IOCTL_MAX     3

#endif // ___nirlpk_ioctl_h___
