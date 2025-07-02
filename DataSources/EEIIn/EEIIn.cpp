
/*---------------------------------------------------------------------------*/
/*                         Standard header includes                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                         Project header includes                           */
/*---------------------------------------------------------------------------*/
#include "AdvancedErrorManagement.h"
#include "CLASSMETHODREGISTER.h"
#include "EEIIn.h"
#include <stdio.h>
/*---------------------------------------------------------------------------*/
/*                           Static definitions                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                           Method definitions                              */
/*---------------------------------------------------------------------------*/

#define DEBUG


namespace MARTe {


EEIIn::EEIIn() : DataSourceI(),
        MessageI(), executor(*this)  {

    offsets = NULL_PTR(uint32*);
    sampleByteSizes = NULL_PTR(uint32*);
    buffer = NULL_PTR(char8*);
    udpBuffer = NULL_PTR(char8*);
    dataSourceMemory = NULL_PTR(char8*);
     if (!eventSem.Create()) {
        REPORT_ERROR(ErrorManagement::FatalError, "Could not create EventSem.");
    }
    mutex.Create();

    sampleReady = false;
}

EEIIn::~EEIIn() {
    if (offsets != NULL_PTR(uint32 *)) {
        GlobalObjectsDatabase::Instance()->GetStandardHeap()->Free(reinterpret_cast<void *& >(offsets));
    }
    if (sampleByteSizes != NULL_PTR(uint32 *)) {
        GlobalObjectsDatabase::Instance()->GetStandardHeap()->Free(reinterpret_cast<void *& >(sampleByteSizes));
    }
     if (buffer != NULL_PTR(char8 *)) {
        GlobalObjectsDatabase::Instance()->GetStandardHeap()->Free(reinterpret_cast<void *&>(buffer));
    }
     if (udpBuffer != NULL_PTR(char8 *)) {
        GlobalObjectsDatabase::Instance()->GetStandardHeap()->Free(reinterpret_cast<void *&>(udpBuffer));
    }
    if (dataSourceMemory != NULL_PTR(char8 *)) {
        GlobalObjectsDatabase::Instance()->GetStandardHeap()->Free(reinterpret_cast<void *&>(dataSourceMemory));
    }
    eventSem.Close();
}

bool EEIIn::AllocateMemory() {
    return true;
}

uint32 EEIIn::GetNumberOfMemoryBuffers() {
    return 1u;
}

bool EEIIn::GetSignalMemoryBuffer(const uint32 signalIdx, const uint32 bufferIdx, void*& signalAddress) {
    bool ok = (dataSourceMemory != NULL_PTR(char8 *));
    if (ok) {
        /*lint -e{613} dataSourceMemory cannot be NULL here*/
        char8 *memPtr = &dataSourceMemory[offsets[signalIdx]];
        signalAddress = reinterpret_cast<void *&>(memPtr);
    }
    return ok;
}

const char8* EEIIn::GetBrokerName(StructuredDataI& data, const SignalDirection direction) {
    const char8* brokerName = "";
    if (direction == InputSignals) {
            brokerName = "MemoryMapSynchronisedInputBroker";
    }
    return brokerName;
}

bool EEIIn::GetOutputBrokers(ReferenceContainer& inputBrokers, const char8* const functionName, void* const gamMemPtr) {
    return false;
}

bool EEIIn::IsSupportedBroker(const SignalDirection direction, const uint32 functionIdx, const uint32 functionSignalIdx, const char8* const brokerClassName)
{
     return true;
}

bool EEIIn::GetInputBrokers(ReferenceContainer& inputBrokers, const char8* const functionName, void* const gamMemPtr) {
  
    bool ok = true;

    ReferenceT<MemoryMapSynchronisedInputBroker> broker("MemoryMapSynchronisedInputBroker");
    ok = broker.IsValid();
    if (ok) {
        ok = broker->Init(InputSignals, *this, functionName, gamMemPtr);
    }

    if (ok) {
	ok = inputBrokers.Insert(broker);
    }
   return ok;
}

bool EEIIn::Synchronise() {
#ifdef DEBUG
    printf("EEIIn::Synchronise\n");
#endif
    mutex.FastLock();
#ifdef DEBUG
    printf("Data Received!\n");
#endif

    if(isSynch)
    {
        while(!sampleReady)
        {
            mutex.FastUnLock();
            eventSem.ResetWait(TTInfiniteWait);
            mutex.FastLock();
        }
        sampleReady = false;
    }
    memcpy(dataSourceMemory, buffer, totBufBytes);
    mutex.FastUnLock();
   return true;
}
  
 
/*lint -e{715}  [MISRA C++ Rule 0-1-11], [MISRA C++ Rule 0-1-12]. Justification: NOOP at StateChange, independently of the function parameters.*/
bool EEIIn::PrepareNextState(const char8* const currentStateName, const char8* const nextStateName) {
    return true;
}

bool EEIIn::Initialise(StructuredDataI& data) {
    bool ok = DataSourceI::Initialise(data);
    if (ok) {
         ok = data.Read("CpuMask", cpuMask);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::Information, "CpuMask not specified for EEIIn - set to 0xff");
            cpuMask = 0xff;
            ok = true;
        }
    }
    if(ok) {
        uint32 synchIn;
        ok = data.Read("IsSync", synchIn);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "IsSync shall be specified");
        }
        isSynch = synchIn > 0;
    }
    if(ok) {
        ok = data.Read("CircuitId", circuitId);
       if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "CircuitId shall be specified");
        }
    }
    if(ok) {
        ok = data.Read("Ip", ipAddr);
       if (!ok) {
            REPORT_ERROR(ErrorManagement::Information, "Ip not specified. Locfalhost assumed.");
            ipAddr = "localhost";
            ok = true;
        }
    }
    if(ok) {
        ok = data.Read("Port", port);
       if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "Port shall be specified");
        }
    }
    return ok;
}


bool EEIIn::SetConfiguredDatabase(StructuredDataI& data) {
     bool ok = DataSourceI::SetConfiguredDatabase(data);
    //Check signal properties and compute memory
    if (ok) { // Check that only one GAM is Connected to the EEIIn
        uint32 auxNumberOfFunctions = GetNumberOfFunctions();
        ok = (auxNumberOfFunctions == 1u);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "Exactly one Function allowed to interact with this EEIIn DataSource. Number of Functions = %u",
                         auxNumberOfFunctions);
        }
    }
    if(ok)
    {
        uint32 nOfSignals = GetNumberOfSignals();
        ok = nOfSignals >= 1;
        if(!ok)
        {
            REPORT_ERROR(ErrorManagement::ParametersError, "At least one signal (Time) shall be specified.");
            return false;
        }
        StreamString currSignalName;
        GetSignalName(0, currSignalName);
        if(isSynch && currSignalName != "Time")
        {
            REPORT_ERROR(ErrorManagement::ParametersError, "The fist signal in synchornous mode shall be named Time");
            return false;
        }
        ok = GetSignalType(0) == SignedInteger32Bit || (GetSignalType(0) == UnsignedInteger32Bit);
        if(!ok)
        {
            REPORT_ERROR(ErrorManagement::ParametersError, "Type of Time shall be either int32 or uint32");
            return false;
        }
        offsets = reinterpret_cast<uint32 *>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(nOfSignals * sizeof(int32)));
        totBufBytes = 0;
        for(uint32 sigIdx = 0; sigIdx < nOfSignals; sigIdx++)
        {
            uint32 nBytes;
            offsets[sigIdx] = totBufBytes;
        	ok = GetSignalByteSize(sigIdx, nBytes);
		    if (!ok) {
                REPORT_ERROR(ErrorManagement::ParametersError, "Error while GetSignalByteSize() for signal %u", sigIdx);
                return ok;
		    }
            uint32 signalSamples;
            ok = GetFunctionSignalSamples(InputSignals, 0u, sigIdx, signalSamples);
		    if (!ok) {
                REPORT_ERROR(ErrorManagement::ParametersError, "Error while GetFunctionSignalSamples() for signal %u", sigIdx);
                return ok;
		    }
            if(signalSamples != 1)
            {
                REPORT_ERROR(ErrorManagement::ParametersError, "Only 1 sample allowed for signal %u", sigIdx);
                return false;
            }
		    totBufBytes += nBytes;
        }
        if(isSynch)
        {
        //Time is derived from header
            expectedPacketLen = totBufBytes + HEADER_LEN - sizeof(int32);
        }
        else
        {
            expectedPacketLen = totBufBytes + HEADER_LEN;
        }

        dataSourceMemory = reinterpret_cast<char8 *>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(totBufBytes * sizeof(char8)));
        buffer = reinterpret_cast<char8 *>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(totBufBytes * sizeof(char8)));
        memset(buffer, 0, totBufBytes);
        //Time is derived from header
        udpBuffer = reinterpret_cast<char8 *>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(expectedPacketLen)); 
	}
    if(ok) {
        if(!udpSocket.Open())
        {
            REPORT_ERROR(ErrorManagement::ParametersError, "Cannot Open UDP socket");
            ok = false;
        }
    }
    if(ok)
    {
        InternetHost ip(port, ipAddr.Buffer());
        udpSocket.SetSource(ip);
        if(!udpSocket.Listen(port))
        {
            REPORT_ERROR(ErrorManagement::ParametersError, "Cannot bind to port %d", port);
            ok = false;
        }
    }
    if(ok)
    {
        //Start listening thread
        if (executor.GetStatus() == EmbeddedThreadI::OffState) {
            if (cpuMask != 0u) {
                executor.SetPriorityClass(Threads::RealTimePriorityClass);
                executor.SetCPUMask(cpuMask);
            }
            executor.SetName(GetName());
            ok = executor.Start();
            if(!ok)
            {
                REPORT_ERROR(ErrorManagement::ParametersError, "Cannot start listener thread");
            }
        }
    }
   return ok;
}




uint32 EEIIn::GetNumberOfBuffers() const {
    return 1;
}

//Message reception management
ErrorManagement::ErrorType EEIIn::Execute(ExecutionInfo& info) {
    ErrorManagement::ErrorType err(true);
    if (info.GetStage() == ExecutionInfo::TerminationStage) {
    }
    else if (info.GetStage() == ExecutionInfo::StartupStage) {
    }
    else {
        uint32 packetLen = expectedPacketLen;
#ifdef DEBUG
        printf("Rading %d bytes from port %d....\n", packetLen, port);
#endif
        udpSocket.Read(udpBuffer, packetLen);
#ifdef DEBUG
        printf("Read!\n");
#endif
        bool ok = packetLen == expectedPacketLen;
        if(!ok) 
        {
            REPORT_ERROR_STATIC(ErrorManagement::FatalError, "Unexpected Datagrame length: %d", packetLen);
            return err;
        }
        uint32 packetId = *(uint32 *)(&udpBuffer[0]);
        char currSigName[257];
#ifdef DEBUG
        printf("Packet Id: %d\t circuitId: %d\n", packetId, circuitId);
#endif
        if(packetId == circuitId)  //The message was for this one
        {
            if(isSynch)
            {
                //Take time from header
                memcpy(buffer, &udpBuffer[2*sizeof(int32)], sizeof(int32));
                //Copy remaining 
                memcpy(&buffer[sizeof(int32)], &udpBuffer[HEADER_LEN], expectedPacketLen - HEADER_LEN);
                sampleReady = true;
                err = !eventSem.Post();
            }
            else
            {
                //simply copy payload
                memcpy(buffer, &udpBuffer[HEADER_LEN], expectedPacketLen - HEADER_LEN);
            }
            mutex.FastUnLock();
        }
    }
    return err;
}


CLASS_REGISTER(EEIIn, "1.0")
}

