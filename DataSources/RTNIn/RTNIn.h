/**
 * @file RTNIn.h
 * @brief Header file for class RTNIn
 * @date 17 Jan 2024
 * @author Gabriele Manduchi
 *
 *
 * @warning Unless required by applicable law or agreed to in writing, 
 * software distributed under the Licence is distributed on an "AS IS"
 * basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the Licence permissions and limitations under the Licence.

 * @details This header file contains the declaration of the class RTNIn
 * with all of its public, protected and private members. It may also include
 * definitions for inline methods which need to be visible to the compiler.
 */

#ifndef RTNIN_H_
#define RTNIN_H_

/*---------------------------------------------------------------------------*/
/*                        Standard header includes                           */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                        Project header includes                            */
/*---------------------------------------------------------------------------*/
#include "DataSourceI.h"
#include "ProcessorType.h"
#include "MemoryMapSynchronisedInputBroker.h"
#include "HttpDataExportI.h"
#include "MessageI.h"
#include "RegisteredMethodsMessageFilter.h"
#include "EventSem.h"
#include "UDPSocket.h"
#include "EmbeddedServiceMethodBinderI.h"
#include "SingleThreadService.h"
/*---------------------------------------------------------------------------*/
/*                           Class declaration                               */
/*---------------------------------------------------------------------------*/
namespace MARTe {
/**
 * @brief A DataSourceI interface which allows Receiving a stream of incoming data via UDP Multicast 
 * events.
 *
 * @details the stream is received in a separate thread (whose CPU mask is exposed as a
 * parameter)and temporarily stored in a  buffer. 
 * Two modes of operation are spported: Synchronous and asycnhronous. 
 * In synchronous mode method Syncronize will return when  
 * the expected number of samples (can be different from signal to signal) for ALL declared signals 
 * has been received. 
 * In asynchronous mode only numSamples = 1 is allowed
 * and Synchronize() returns soon, possibly leaving the previous samples. 
 * The received datagram wil include a 16 Bytes header defined as follows:
 *  - Circuit identifier (4 bytes)
 *  - Free running counter (4 bytes) 
 *  - Time within experiment (us, 4 bytes)
 *  - Status information (4 bytes)
 *  - Absolute time of send (PTP, 8 bytes)
 *
 * The circuit identifier shall correspond to the declared Id parameter. Otherwise the message is discarded.
 *  
 * The message payload will bring information about a single sample for a (sub)set of declared signals 
 * An error is raised is a signal other than the declared signals is received
 * the payload is organized in the following way:
 * - Number of signals (2 bytes)
 * - For each signal:
 *      - name len (1 byte)
 *      - name (len bytes)
 *      - sample len (2 bytes)
 *      - sample (sample len bytes)
 * 
 * The following parameters are defined:
 * - IpAddr: IP address for this receiver
 * - Port: receive port
 * - Id: circuit identifier selector
 * - IsSync: Synchronous/Asynchronous flag (1/0)
 * 
 *
 * */

class RTNIn: public DataSourceI, public MessageI, public EmbeddedServiceMethodBinderI {
public:
    CLASS_REGISTER_DECLARATION()

    /**
     * @brief Default constructor.
     * @details 
     */
    RTNIn();

    /**
     * @brief Destructor.
     * @details 
     */
    virtual ~RTNIn();

    /**
     * @brief See DataSourceI::AllocateMemory. NOOP.
     * @return true.
     */
    virtual bool AllocateMemory();

    /**
     * @brief See DataSourceI::GetNumberOfMemoryBuffers.
     * @return 1.
     */
    virtual uint32 GetNumberOfMemoryBuffers();

    /**
     * @brief See DataSourceI::GetSignalMemoryBuffer.
     * @pre
     *   SetConfiguredDatabase
     */
    virtual bool GetSignalMemoryBuffer(const uint32 signalIdx,
            const uint32 bufferIdx,
            void *&signalAddress);

    /**
     * @brief See DataSourceI::GetBrokerName.
     * @details Only OutputSignals are supported.
     * @return MemoryMapSynchInputBroker.
     */
    virtual const char8 *GetBrokerName(StructuredDataI &data,
            const SignalDirection direction);

    /**
     * @brief See DataSourceI::GetInputBrokers.
     * @return MemoryMapSynchInputBroker.
     */
    virtual bool GetInputBrokers(ReferenceContainer &inputBrokers,
            const char8* const functionName,
            void * const gamMemPtr);

    /**
     * @brief See DataSourceI::GetOutputBrokers.
     * @retrun false
     */
    virtual bool GetOutputBrokers(ReferenceContainer &outputBrokers,
            const char8* const functionName,
            void * const gamMemPtr);

    /**
     * @brief Wait and copy for data in circular buffer into data buffer.
     * @return true if the data has been received
     */
    virtual bool Synchronise();

    /**
     * @brief See DataSourceI::PrepareNextState. NOOP.
     * @return true.
     */
    virtual bool PrepareNextState(const char8 * const currentStateName,
            const char8 * const nextStateName);

    /**
     * @brief Loads and verifies the configuration parameters detailed in the class description.
     * @return true if all the mandatory parameters are correctly specified and if the specified optional parameters have valid values.
     */
    virtual bool Initialise(StructuredDataI & data);

    /**
     * @brief Final verification of all the parameters and activation of receiver thread
     * @details This method verifies that all the parameters 
     *  are valid and consistent with the parameters set during the initialisation phase.
     * In particular the following conditions shall be met:
     * - The number of dimension of all the signals is zero.
     * - At least one signal is set.
     * @return true if all the parameters are valid and if the thread is successfully opened.
     */
    virtual bool SetConfiguredDatabase(StructuredDataI & data);

    /**
     * @brief Check if the brober is supported
     * @return true if the brober is supported
     */
    virtual bool IsSupportedBroker(const SignalDirection direction, const uint32 functionIdx, 
				   const uint32 functionSignalIdx, const char8* const brokerClassName);

    /**
     * @brief Gets the number of buffers in the circular buffer.
     * @return the number of buffers in the circular buffer.
     */
    uint32 GetNumberOfBuffers() const;

    ErrorManagement::ErrorType Execute(ExecutionInfo& info);
private:
     /**
     * Array of signal names in the order in which  they are declared in DataSource memory
    */
     StreamString *signalNames;
    /**
     * Number of declared samples for every signal
    */
   uint32 *samples;
    /**
     * Current number of received samples
    */
    uint32 *currSamples;

    /**
     * Offset of each signal in the internal buffer and dataSourceMemory
     */
    uint32 *offsets;

    /**
     * Size in bytes of every signal sample
     */
    uint32 *sampleByteSizes;

    /**
     * Return the index in dataSourcememory of the signal specified by name
    */
    int getSignalId(char *sigName);

    /**
     * Internal buffer hosting incoming samples
    */
   char8 *buffer;
    /**
     * Memory holding all the signals that are to be stored, for each cycle, in the output file.
     */
    char8 *dataSourceMemory;

    /**
     * UDP receive buffer
    */
    char8 *udpBuffer;

    uint32 totBufBytes;
    bool samplesReady;
    bool isSynch;
    uint32 cpuMask;
    uint21 stackSize;
    uint32 nOfSignals; 
    uint32 maxPacketLen;
    EventSem eventSem;
    FastPollingMutexSem mutex;
    SingleThreadService executor;

    StreamString ipAddr;
    uint32 port;
    uint32 circuitId;

    UDPSocket udpSocket;
  };
  

}


/*---------------------------------------------------------------------------*/
/*                        Inline method definitions                          */
/*---------------------------------------------------------------------------*/

#endif /* RTNIN_H_ */
	
