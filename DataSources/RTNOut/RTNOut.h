
#ifndef RTNPOUT_H_
#define RTNOUT_H_

/*---------------------------------------------------------------------------*/
/*                        Standard header includes                           */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                        Project header includes                            */
/*---------------------------------------------------------------------------*/
#include "DataSourceI.h"
#include "ProcessorType.h"
#include "MemoryMapSynchronisedOutputBroker.h"
#include "MessageI.h"
#include "RegisteredMethodsMessageFilter.h"
#include <UDPSocket.h>

/**
 * RTNOut DataSource allows sending an UDP Datagram containing the packeted version of the current sample for all its
 * declared inputs. The UDP Datagram starts with the following header
 *  - Circuit identifier (4 bytes)
 *  - Free running counter (4 bytes) 
 *  - Time within experiment (us, 4 bytes) (not used yet)
 *  - Status information (4 bytes)  (not used yet)
 *  - Absolute time of send (PTP, 8 bytes) (not used yet)
 * The input signals are packed as follows:
 * - Number of signals (2 bytes)
 * - For each signal:
 *      - name len (1 byte)
 *      - name (len bytes)
 *      - sample len (2 bytes)
 *      - sample (sample len bytes)
 * 
 * No ckeck is performed on the signal data type and dimension, but the NumberOfSamples must be 1. They are sent as they are.
 * 
 * The following parameters are defined for every input signal: 
 * Ips: destination Ip Addresses 
 * Ports: destination Ports (must be of the same number of elements of IP Addresses)
 * Id: CircuitIdentifier (unique for signal)
 **/

/*---------------------------------------------------------------------------*/
/*                           Class declaration                               */
/*---------------------------------------------------------------------------*/
namespace MARTe {

class RTNOut: public DataSourceI {
public:
    CLASS_REGISTER_DECLARATION()

    /**
     * @brief Default constructor.
     * @details Initialises all the optional parameters as described in the class description.
     * Registers the RPC FlushFile, CloseFile and OpenFile callback functions.
     */
    RTNOut();

    /**
     * @brief Destructor.
     * @details Flushes the file and frees the circular buffer.
     */
    virtual ~RTNOut();

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
     * @return MemoryMapAsyncOutputBroker if storeOnTrigger == 0, MemoryMapAsyncTriggerOutputBroker otherwise.
     */
    virtual const char8 *GetBrokerName(StructuredDataI &data,
            const SignalDirection direction);

    /**
     * @brief See DataSourceI::GetInputBrokers.
     * @return false.
     */
    virtual bool GetInputBrokers(ReferenceContainer &inputBrokers,
            const char8* const functionName,
            void * const gamMemPtr);

    /**
     * @brief See DataSourceI::GetOutputBrokers.
     * @details If storeOnTrigger == 0 it adds a MemoryMapAsyncOutputBroker instance to
     *  the inputBrokers, otherwise it adds a MemoryMapAsyncTriggerOutputBroker instance to the outputBrokers.
     * @pre
     *   GetNumberOfFunctions() == 1u
     */
    virtual bool GetOutputBrokers(ReferenceContainer &outputBrokers,
            const char8* const functionName,
            void * const gamMemPtr);

    /**
     * @brief Writes the buffer data into the specified file in the specified format.
     * @return true if the data can be successfully written into the file.
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
     */
    virtual bool SetConfiguredDatabase(StructuredDataI & data);

    /**
     * @brief Flushes the file.
     * @return true if the file can be successfully flushed.
     */
    virtual bool IsSupportedBroker(const SignalDirection direction, const uint32 functionIdx, 
				   const uint32 functionSignalIdx, const char8* const brokerClassName);

    /**
     * @brief Gets the number of buffers in the circular buffer.
     * @return the number of buffers in the circular buffer.
     */
    uint32 GetNumberOfBuffers() const;

       
private:

    /**
     * Offset of each signal in the dataSourceMemory
     */
    uint32 **offsets;

    /**
     * Memory holding all the signals that are to be sent.
     */
    char8 *dataSourceMemory;

    /**
     * Array of signal names in the order in which  they are declared in DataSource memory
    */
     StreamString **signalNames;
    /**
     * For every signal, the number of fields (!= 0 is the signal is structured) 
    */
     uint32 *nOfSignalFields;
     

     /**
     * Size in bytes of every signal sample
     */
      uint32 **sampleByteSizes;

   /**
     * The asynchronous triggered broker that provides the interface between the GAMs and the output file.
     */

    uint32 counter;
    uint32 nOfSignals;  
    uint32 *packetLens;
    uint32 maxPacketLen;
    char8 *packet;
    uint32 *numIps;
    uint32 **circuitIds;
    UDPSocket **udpSockets;  


};
}


/*---------------------------------------------------------------------------*/
/*                        Inline method definitions                          */
/*---------------------------------------------------------------------------*/

#endif /* RTNOUT_H_ */
	
