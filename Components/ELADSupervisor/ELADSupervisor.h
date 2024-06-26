
#ifndef ELADSUPERVISOR_H
#define ELADSUPERVISOR_H

/*---------------------------------------------------------------------------*/
/*                        Standard header includes                           */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                        Project header includes                            */
/*---------------------------------------------------------------------------*/
#include "ErrorManagement.h"
#include "EmbeddedServiceI.h"
#include "EmbeddedServiceMethodBinderI.h"
#include "ErrorType.h"
#include "ExecutionInfo.h"
#include "EventSem.h"
#include "FastPollingMutexSem.h"
#include "MessageI.h"
#include "ReferenceContainer.h"
#include "SingleThreadService.h"
#include "StreamString.h"
#include "BasicTCPSocket.h"
#include <iostream>
#include <string>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "picorfxdriver.h"

/*---------------------------------------------------------------------------*/
/*                           Class declaration                               */
/*---------------------------------------------------------------------------*/

namespace MARTe {
/**
 * @brief Supervisor code for ELAD device. It handles device confuguration and ransient recording.
 * When initialized it starts a thread listening to a TCP connection (Port number configurable). 
 * It accepts the following commans:
 * CHK 1 the string CHK is followed by the int 1. If received as is there is no need for endianity conversion. 
 * Otherwise (16777216) conversion is required. It opens the device and return either OK or ER as 2 chars string 
 * PTS <pts> the string PTS is followed by the number of Post Trigger Samples (possibly to be corrected for endianity). 
 * Returns either OK or ER as string
 * DIV <clock division> Set streaming clock division in respect to 1 MHz. Returns either OK or ER as string.
 * AUT <autozero time (float)> defines the Autozero time. Returns either OK or ER as string
 * TEX <0|1> the string TEX is followd by either string '1' or '0' to indicate wheteher external HW trigger is accepted.
 * TAU <0|1> the string TAU is followd by either string '1' or '0' to indicate wheteher external HW autozero trigger is accepted.
 * Returns either OK or ER as string
 * ARM to arm the device.   Returns either OK or ER as string.
 * TRG to provide software trigger. Returns either OK or ER as string.
 * TRS to provide software autozero trigger. Returns either OK or ER as string.
 * STS to start data streaming. Returns either OK or ER as string.
 * STO to stop data streaming. Returns either OK or ER as string.
 * STR to read acquired samples. Returns either OK or ER as string. When OK, the number of samples (PTS) is returned as a 4 byte word, possibly 
 * adapted for endianity, followed by 12 * PTS longowrds corresponding to the 12 channels of the device, possibly adapted for endianity.
 * CLS closes the device. Returns either OK or ER as string.
 * @details 
 */
class ELADSupervisor: public ReferenceContainer, public EmbeddedServiceMethodBinderI, public MessageI {
public:
    CLASS_REGISTER_DECLARATION()
    /**
     * @brief Constructor. NOOP.
     */
    ELADSupervisor    ();

    /**
     * @brief Destructor. Stops the embedded thread.
     */
    virtual ~ELADSupervisor();

    /**
     * @brief Initialises the ReferenceContainer and reads the number of PTS.
     * @return true if the ReferenceContainer and thread parameters are successfully initialised.
     */
    virtual bool Initialise(StructuredDataI & data);

    /**
     * @brief Provides the context to serve incoming TCP messages for configuring, arming, triggering the device and storing darta acquired via DMA
     * @return ErrorManagement::NoError if all the EPICS calls return without any error.
     */
    virtual ErrorManagement::ErrorType Execute( ExecutionInfo & info);

   
    /**
     * @brief Gets the embedded thread state.
     * @return the embedded thread state.
     */
    EmbeddedThreadI::States GetStatus();
  
   /**
     * @brief Gets the thread stack size.
     * @return the thread stack size.
     */
    uint32 GetStackSize() const;

    /**
     * @brief Gets the thread affinity.
     * @return the thread affinity.
     */
    uint32 GetCPUMask() const;


  /**
     * @brief Start the embedded service it wasn't already started.
     * @return ErrorManagement::NoError if the service wasn't already started.
     */
    ErrorManagement::ErrorType Start();
 
 private:
    /** 
     * Handle received command. Returns False on error or on CLS command and exits loop
     */
    bool handleCommand(BasicTCPSocket *sock, char *cmd);
  
    /** 
     * Send message containing fd to ELAD DataSource
     */
    void sendFd();

  /** 
     * Send message containing Out Udp IP and port to ELAD DataSource
     */
    void sendIp();

    /** 
     * Send message for enabling/disabling stram data readout
     */
      void enableStream(bool enable);
    /**
     * Read exacly requested bytes
     * 
    */
    bool readSock(BasicTCPSocket *sock, char *buf, int32 size);

    /**
     * Handle int read
    */
    int shuffle(int data);
    
    /**
     * The EmbeddedThread where the TCP messages are served.
     */
    SingleThreadService executor;

    /**
     * Listening socket
    */
    BasicTCPSocket serverSock;

    /**
     * Endianity flag
    */
    bool endianConversionNeeded;

    /**
     * Listen port
    */
    int32 port;

    /**
     * Thread stack size
    */
    int32 stackSize;

    /**
     * Thread CPU Mask
     */
     int32 cpuMask;

   /**
    * ELAD Device
   */
     int32 fd;

   /**
    * Post Trigger Samples
    * */  
      int32 pts;
    /**
    * Clock division for streaming
    * */  
      int32 clockDiv;
    /**
    * Use HW trigger
    * */  
      bool useHwTrig;
    /**
    * Use HW autozero trigger
    * */  
      bool useHwAutozeroTrig;
    /**
    * Autozero duration
    * */  
      float32 autozeroDuration;

    /**
     * Upd out IP
     * */  
    StreamString udpOutIp;

    /**
     * Upd out Port
     * */  
    uint32 udpOutPort;

    bool ipReceived;
};  



}

/*---------------------------------------------------------------------------*/
/*                        Inline method definitions                          */
/*---------------------------------------------------------------------------*/

#endif /* ELADSUPERVISOR_H */

