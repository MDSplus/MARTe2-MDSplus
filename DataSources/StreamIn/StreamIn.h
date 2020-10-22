/**
 * @file FileWriter.h
 * @brief Header file for class FileWriter
 * @date 11/08/2017
 * @author Andre' Neto
 *
 * @copyright Copyright 2015 F4E | European Joint Undertaking for ITER and
 * the Development of Fusion Energy ('Fusion for Energy').
 * Licensed under the EUPL, Version 1.1 or - as soon they will be approved
 * by the European Commission - subsequent versions of the EUPL (the "Licence")
 * You may not use this work except in compliance with the Licence.
 * You may obtain a copy of the Licence at: http://ec.europa.eu/idabc/eupl
 *
 * @warning Unless required by applicable law or agreed to in writing, 
 * software distributed under the Licence is distributed on an "AS IS"
 * basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the Licence permissions and limitations under the Licence.

 * @details This header file contains the declaration of the class FileWriter
 * with all of its public, protected and private members. It may also include
 * definitions for inline methods which need to be visible to the compiler.
 */

#ifndef STREAMIN_H_
#define STREAMIN_H_

/*---------------------------------------------------------------------------*/
/*                        Standard header includes                           */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                        Project header includes                            */
/*---------------------------------------------------------------------------*/
#include "DataSourceI.h"
#include "ProcessorType.h"
#include "MemoryMapSynchronisedInputBroker.h"
#include "MessageI.h"
#include "RegisteredMethodsMessageFilter.h"
#include "EventSem.h"
#include <mdsobjects.h>

/*---------------------------------------------------------------------------*/
/*                           Class declaration                               */
/*---------------------------------------------------------------------------*/
namespace MARTe {
/**
 * @brief A DataSourceI interface which allows Receiving a stream of incoming data via MDSplus 
 * events.
 *
 * @details the stream is received in a separate thread (whose CPU mask is exposed as a
 * parameter)and temporarily stored in a circular buffer. When the DataSource is activated (i.e.
 * its Synchronize() method is called, if a sample is present, then it is returned. 
 * Two modes of operation are spported: asynchronous and synchronized. In asynchronous mode
 * Synchronize returns soon, possibly leaving the previous sample. 
 * Otherwise, Synchronize() suspends until the reqested number of samples has been received. 
 * If synchronized, a signal named Time shall be defined, that will report the current sample
 * time based on the Period parameter. i.e. time[i] = sampleCount * Period
 * If more than one signal is defined (a channel name is associated with every signal) then 
 * synchronization can be based on the reception for a single channel, or from all channels. 
 * Currently only scalar values can be received, with a settable number of samples (default 1).
 * Type conversion is supported.
 *
 * */

 class StreamListener: public MDSplus::DataStreamListener
 {
    uint32 signalIdx;
    float32 **streamBuffers;
    float32 *timeBuffer;
    uint32 *bufIdxs;
    uint32 *lastBufIdxs;
    uint32 *bufElements;
    EventSem *eventSem;
    FastPollingMutexSem *mutexSem;
    uint32 nOfBuffers;
    bool checkOverflow;
    StreamString channelName;
    bool *started;

public:
    StreamListener(StreamString channelName, uint32 signalIdx, float32 **streamBuffers, uint32 *bufIdxs, uint32 *lastBufIdxs, 
	uint32 *bufElements, EventSem *eventSem, FastPollingMutexSem *mutexSem, uint32 nOfBuffers, bool checkOverflow, float32 *timeBuffer,
	bool *started)
    {
	this->channelName = channelName;
	this->signalIdx = signalIdx;
	this->bufIdxs = bufIdxs;
	this->lastBufIdxs = lastBufIdxs;
	this->bufElements = bufElements;
	this-> eventSem = eventSem;
	this->mutexSem = mutexSem; 
	this->streamBuffers = streamBuffers;
	this->bufElements = bufElements;
	this->nOfBuffers = nOfBuffers;
	this->checkOverflow = checkOverflow;
	this->timeBuffer = timeBuffer;
	this->started = started;
    } 
    virtual ~StreamListener() {}
    virtual void dataReceived(MDSplus::Data *samples, MDSplus::Data *times, int shot);
  };



class StreamIn: public DataSourceI, public MessageI {
public:
    CLASS_REGISTER_DECLARATION()

    /**
     * @brief Default constructor.
     * @details 
     */
    StreamIn();

    /**
     * @brief Destructor.
     * @details 
     */
    virtual ~StreamIn();

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

  
private:

    /**
     * Offset of each signal in the dataSourceMemory
     */
    uint32 *offsets;

    /**
     * Memory holding all the signals that are to be stored, for each cycle, in the output file.
     */
    char8 *dataSourceMemory;


    uint32 numberOfBuffers;
    uint32 cpuMask;
    uint21 stackSize;
    uint32 nOfSignals; 
    uint32 *bufElements;
    uint32 *bufIdxs;
    uint32 *lastBufIdxs;
    uint32 numChannels;
    float32 **streamBuffers;
    float32 *synchStreamTime;
    EventSem eventSem;
    FastPollingMutexSem mutexSem;
    int32 synchronizingIdx;
    StreamString *channelNames;
    MDSplus::EventStream evStream;
    StreamListener **streamListeners;
    uint32 counter;
    float32 period;
    bool started;
 };
  

}


/*---------------------------------------------------------------------------*/
/*                        Inline method definitions                          */
/*---------------------------------------------------------------------------*/

#endif /* STREAMIN_H_ */
	
