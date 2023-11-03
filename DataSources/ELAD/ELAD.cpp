/**
 * @file ELAD.cpp
 * @brief Source file for class ELAD
 * @date 9/28/2023
 * @author Gabriele manduchi
 *
 * */

/*---------------------------------------------------------------------------*/
/*                         Standard header includes                          */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/*                         Project header includes                           */
/*---------------------------------------------------------------------------*/
#include "AdvancedErrorManagement.h"
#include "MemoryMapSynchronisedInputBroker.h"
#include "ELAD.h"
#include <unistd.h>
#include "CLASSMETHODREGISTER.h"
#include <stdio.h>
#define NUM_ELAD_CHANS 2
/*---------------------------------------------------------------------------*/
/*                           Static definitions                              */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/*                           Method definitions                              */
/*---------------------------------------------------------------------------*/
namespace MARTe {
ELAD::ELAD() : DataSourceI() {
     fd = -1;
    if (!synchSem.Create()) 
    {
        REPORT_ERROR(ErrorManagement::FatalError, "Could not create EventSem.");
    }
    if (!streamSem.Create()) 
    {
        REPORT_ERROR(ErrorManagement::FatalError, "Could not create EventSem.");
    }
    streamEnabled = false;
}



ELAD::~ELAD() {
 }


bool ELAD::AllocateMemory() {
    return true;
}

uint32 ELAD::GetNumberOfMemoryBuffers() {
    return 1u;
}
bool ELAD::GetSignalMemoryBuffer(const uint32 signalIdx, const uint32 bufferIdx, void*& signalAddress) {
    bool ok;
    ok = (signalIdx < 2 * ELAD_CHANNELS);
    if (ok) {
	    signalAddress = static_cast<void *> (&inputChanBuffers[signalIdx]);
    }
    return ok;
}
bool ELAD::IsSupportedBroker(const SignalDirection direction, const uint32 functionIdx, const uint32 functionSignalIdx, const char8* const brokerClassName)
{
     return true;
}

const char8* ELAD::GetBrokerName(StructuredDataI& data, const SignalDirection direction) {
    const char8 *brokerName = NULL_PTR(const char8 *);
    if (direction == InputSignals) 
    {
        brokerName = "MemoryMapSynchronisedInputBroker";
    }
    return brokerName;
}

bool ELAD::GetInputBrokers(ReferenceContainer& inputBrokers, const char8* const functionName, void* const gamMemPtr) {
  REPORT_ERROR(ErrorManagement::Debug, "GetInputBrokers");
    ReferenceT<MemoryMapSynchronisedInputBroker> broker("MemoryMapSynchronisedInputBroker");
    bool ok = broker.IsValid();

    if (ok) {
        ok = broker->Init(InputSignals, *this, functionName, gamMemPtr);
    }
    if (ok) {
        ok = inputBrokers.Insert(broker);
    }
    return ok;
}

bool ELAD::GetOutputBrokers(ReferenceContainer& outputBrokers, const char8* const functionName, void* const gamMemPtr) {
    REPORT_ERROR(ErrorManagement::Debug, "GetOutputBrokers");   
	return false;
}

bool ELAD::PrepareNextState(const char8* const currentStateName, const char8* const nextStateName) {
    return true;
}

bool ELAD::Initialise(StructuredDataI& data) {
    bool ok = DataSourceI::Initialise(data);
    if(ok)
    {
        ErrorManagement::ErrorType err(true);
        if(!synchSem.Reset()) 
        {
            REPORT_ERROR(ErrorManagement::FatalError, "Error resetting Semathore");
            return false;
        }        
        if(!streamSem.Reset()) 
        {
            REPORT_ERROR(ErrorManagement::FatalError, "Error resetting Semathore");
            return false;
        }        
    }
    if(ok)
    {
        ReferenceT<RegisteredMethodsMessageFilter> filter = ReferenceT<RegisteredMethodsMessageFilter>(GlobalObjectsDatabase::Instance()->GetStandardHeap());
        filter->SetDestination(this);
        ErrorManagement::ErrorType ret = MessageI::InstallMessageFilter(filter);
        if (!ret.ErrorsCleared()) {
            ok = false;
            REPORT_ERROR(ErrorManagement::FatalError, "Failed to install message filters");
        }
    }

    return ok;
}

bool ELAD::SetConfiguredDatabase(StructuredDataI& data) {
    bool ok = DataSourceI::SetConfiguredDatabase(data);
    if(!ok) return ok;

    uint32 nOfFunctions = GetNumberOfFunctions();
    if(nOfFunctions > 1)
    {
	    REPORT_ERROR(ErrorManagement::ParametersError, "Only one function allowed");
	    ok = false;
	    return ok;
    }
    uint32 nOfSignals = 0u;
    nOfSignals = GetNumberOfSignals();
    if(nOfSignals != 2* NUM_ELAD_CHANS)
    {
	    REPORT_ERROR(ErrorManagement::ParametersError, "The declared number %d of signals shall be %d",nOfSignals, 2 * NUM_ELAD_CHANS);
	    return false;
    }
    for(uint32 sigIdx = 0; sigIdx < nOfSignals; sigIdx++)
    {
        if(ok)
        {
	        TypeDescriptor sigType = GetSignalType(sigIdx);
	        ok = (sigType == SignedInteger32Bit);
            if (!ok) {
                REPORT_ERROR(ErrorManagement::ParametersError, "Signal type shall be Int32");
            }
        }
        uint32 nElements = -1;
        if(ok)
        {
	        GetSignalNumberOfElements(sigIdx, nElements);
	        ok = (nElements == 1);
            if (!ok) {
                REPORT_ERROR(ErrorManagement::ParametersError, "Number of elements shall be 1");
 	        }
        }
    }
    return ok;
}

bool ELAD:: Synchronise()
{
    if(fd == -1) //File Descriptor not yet received
    {
        ErrorManagement::ErrorType err(true);
        printf("WAITING FD......");
        if(!synchSem.Wait(TTInfiniteWait))
        {
            REPORT_ERROR(ErrorManagement::FatalError, "Error Waiting Semaphore");
            return false;
        }        
        printf("GOT FD: %d\n", fd);
    }


    while(!streamEnabled)
    {
        printf("Waiting to be enabled to stream.....\n");
        if(!streamSem.Wait(TTInfiniteWait))
        {
            REPORT_ERROR(ErrorManagement::FatalError, "Error Waiting Semaphore");
            return false;
        }        
    }
    memset(inputChanBuffers, 0, sizeof(inputChanBuffers));
    uint32  currSize = 0;
    while((currSize < 2*NUM_ELAD_CHANS * sizeof(int)) && streamEnabled)
    {
       // printf("Reading.....\n");
        int rb = read(fd, ((char *)inputChanBuffers)+currSize, 2*NUM_ELAD_CHANS*sizeof(int) - currSize);
      //  if(rb > 0) printf("READ %d\n", rb);
        if(rb < 0)
        {
          printf("ERROR READING FD\n");
          return NULL;
        }
        currSize += rb;
    }
    printf("%d\n", inputChanBuffers[0]);
    return true;
}

ErrorManagement::ErrorType  ELAD::setFd(int32 fd)
{
    printf("ARRIVATO FD %d\n", fd);
    this->fd = fd;
    synchSem.Post();
    return ErrorManagement::NoError;
}

 ErrorManagement::ErrorType ELAD::enableStream(int32 enable)
 {
    if(enable)
    {
        streamEnabled = true;
        streamSem.Post();
    }
    else
    {
        streamSem.Reset();
        streamEnabled = false;
    }
    return ErrorManagement::NoError;
 }
   

CLASS_REGISTER(ELAD, "1.0")
CLASS_METHOD_REGISTER(ELAD, setFd)
CLASS_METHOD_REGISTER(ELAD, enableStream)


}

