
/*---------------------------------------------------------------------------*/
/*                         Standard header includes                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                         Project header includes                           */
/*---------------------------------------------------------------------------*/
#include "AdvancedErrorManagement.h"
#include "CLASSMETHODREGISTER.h"
#include "RTNIn.h"
#include <stdio.h>
/*---------------------------------------------------------------------------*/
/*                           Static definitions                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                           Method definitions                              */
/*---------------------------------------------------------------------------*/
namespace MARTe {


RTNIn::RTNIn() : DataSourceI(),
        MessageI(), executor(*this)  {

    signalNames = NULL_PTR(StreamString *);
    samples = NULL_PTR(uint32*);
    currSamples = NULL_PTR(uint32*);
    offsets = NULL_PTR(uint32*);
    sampleByteSizes = NULL_PTR(uint32*);
    buffer = NULL_PTR(char8*);
    udpBuffer = NULL_PTR(char8*);
    dataSourceMemory = NULL_PTR(char8*);
     if (!eventSem.Create()) {
        REPORT_ERROR(ErrorManagement::FatalError, "Could not create EventSem.");
    }
    mutex.Create();

    samplesReady = false;
}

RTNIn::~RTNIn() {
    if (signalNames != NULL_PTR(StreamString *)) {
        delete []signalNames;
    }
    if (samples != NULL_PTR(uint32 *)) {
        GlobalObjectsDatabase::Instance()->GetStandardHeap()->Free(reinterpret_cast<void *& >(samples));
    }
    if (currSamples != NULL_PTR(uint32 *)) {
        GlobalObjectsDatabase::Instance()->GetStandardHeap()->Free(reinterpret_cast<void *& >(currSamples));
    }
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

bool RTNIn::AllocateMemory() {
    return true;
}

uint32 RTNIn::GetNumberOfMemoryBuffers() {
    return 1u;
}

bool RTNIn::GetSignalMemoryBuffer(const uint32 signalIdx, const uint32 bufferIdx, void*& signalAddress) {
    bool ok = (dataSourceMemory != NULL_PTR(char8 *));
    if (ok) {
        /*lint -e{613} dataSourceMemory cannot be NULL here*/
        char8 *memPtr = &dataSourceMemory[offsets[signalIdx]];
        signalAddress = reinterpret_cast<void *&>(memPtr);
    }
    return ok;
}

const char8* RTNIn::GetBrokerName(StructuredDataI& data, const SignalDirection direction) {
    const char8* brokerName = "";
    if (direction == InputSignals) {
            brokerName = "MemoryMapSynchronisedInputBroker";
    }
    return brokerName;
}

bool RTNIn::GetOutputBrokers(ReferenceContainer& inputBrokers, const char8* const functionName, void* const gamMemPtr) {
    return false;
}

bool RTNIn::IsSupportedBroker(const SignalDirection direction, const uint32 functionIdx, const uint32 functionSignalIdx, const char8* const brokerClassName)
{
     return true;
}

bool RTNIn::GetInputBrokers(ReferenceContainer& inputBrokers, const char8* const functionName, void* const gamMemPtr) {
  
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

bool RTNIn::Synchronise() {
#ifdef DEBUG
    REPORT_ERROR(ErrorManagement::Debug, "Synchronise");
#endif
    mutex.FastLock();
    while(!samplesReady)
    {
        mutex.FastUnLock();
        eventSem.ResetWait(TTInfiniteWait);
        mutex.FastLock();
    }
    memcpy(dataSourceMemory, buffer, totBufBytes);
    mutex.FastUnLock();
   return true;
}
  
 
/*lint -e{715}  [MISRA C++ Rule 0-1-11], [MISRA C++ Rule 0-1-12]. Justification: NOOP at StateChange, independently of the function parameters.*/
bool RTNIn::PrepareNextState(const char8* const currentStateName, const char8* const nextStateName) {
    return true;
}

bool RTNIn::Initialise(StructuredDataI& data) {
    bool ok = DataSourceI::Initialise(data);
    if (ok) {
         ok = data.Read("CpuMask", cpuMask);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::Information, "CpuMask not specified for RTNIn");
            cpuMask = 0xff;
            ok = true;
        }
    }
    if(ok) {
        uint32 synchIn;
        ok = data.Read("IsSynch", synchIn);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::Information, "IsSynch shall be specified");
        }
        isSynch = synchIn > 0;
    }
    if(ok) {
        ok = data.Read("CircuitId", circuitId);
       if (!ok) {
            REPORT_ERROR(ErrorManagement::Information, "Id shall be specified");
        }
    }
    if(ok) {
        ok = data.Read("Ip", ipAddr);
       if (!ok) {
            REPORT_ERROR(ErrorManagement::Information, "IpAddr shall be specified");
        }
    }
    if(ok) {
        ok = data.Read("Port", port);
       if (!ok) {
            REPORT_ERROR(ErrorManagement::Information, "Port shall be specified");
        }
    }
    if(ok)
    {
        ok = data.MoveRelative("Signals");
        if(!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError,"Signals node Missing.");
            return ok;
        }
        nOfSignals = data.GetNumberOfChildren();
        signalNames = new StreamString[nOfSignals];
        for (uint32 sigIdx = 0; sigIdx < nOfSignals; sigIdx++) {
            signalNames[sigIdx] = data.GetChildName(sigIdx);
        }
        data.MoveToAncestor(1u);
    }
    return ok;
}


bool RTNIn::SetConfiguredDatabase(StructuredDataI& data) {
     bool ok = DataSourceI::SetConfiguredDatabase(data);
    //Check signal properties and compute memory
    if (ok) { // Check that only one GAM is Connected to the RTNIn
        uint32 auxNumberOfFunctions = GetNumberOfFunctions();
        ok = (auxNumberOfFunctions == 1u);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "Exactly one Function allowed to interact with this RTNIn DataSource. Number of Functions = %u",
                         auxNumberOfFunctions);
        }
    }
    if(ok)
    {
        samples = reinterpret_cast<uint32 *>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(nOfSignals * sizeof(int32)));
        currSamples = reinterpret_cast<uint32 *>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(nOfSignals * sizeof(int32)));
        sampleByteSizes = reinterpret_cast<uint32 *>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(nOfSignals * sizeof(int32)));
        offsets = reinterpret_cast<uint32 *>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(nOfSignals * sizeof(int32)));
        totBufBytes = 0;
        maxPacketLen = 16 + sizeof(uint16);
        for(uint32 sigIdx = 0; sigIdx < nOfSignals; sigIdx++)
        {
            uint32 nBytes;
            offsets[sigIdx] = totBufBytes;
        	ok = GetSignalByteSize(sigIdx, nBytes);
		    if (!ok) {
                REPORT_ERROR(ErrorManagement::ParametersError, "Error while GetSignalByteSize() for signal %u", sigIdx);
                return ok;
		    }
		    totBufBytes += nBytes;
            currSamples[sigIdx] = 0;
            ok = GetFunctionSignalSamples(InputSignals, 0u, sigIdx, samples[sigIdx]);
		    if (!ok) {
                REPORT_ERROR(ErrorManagement::ParametersError, "Error while GetFunctionSignalSamples() for signal %u", sigIdx);
                return ok;
		    }
            sampleByteSizes[sigIdx] = nBytes/samples[sigIdx];
            maxPacketLen += sampleByteSizes[sigIdx] + sizeof(uint16)+sizeof(uint8)+signalNames[sigIdx].Size();
        }
        dataSourceMemory = reinterpret_cast<char8 *>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(totBufBytes * sizeof(char8)));
        buffer = reinterpret_cast<char8 *>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(totBufBytes * sizeof(char8)));
        udpBuffer = reinterpret_cast<char8 *>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(maxPacketLen * sizeof(char8)));
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




uint32 RTNIn::GetNumberOfBuffers() const {
    return 1;
}

//Message reception management
ErrorManagement::ErrorType RTNIn::Execute(ExecutionInfo& info) {
    ErrorManagement::ErrorType err(true);
    if (info.GetStage() == ExecutionInfo::TerminationStage) {
    }
    else if (info.GetStage() == ExecutionInfo::StartupStage) {
    }
    else {
     //   printf("READING %d BYTES....\n", packetLen);
        uint32 packetLen = maxPacketLen;
        udpSocket.Read(udpBuffer, packetLen);
        bool ok = packetLen > 16 + sizeof(int16); //Limit case 0 signals
        if(!ok) 
        {
            REPORT_ERROR_STATIC(ErrorManagement::FatalError, "Unexpected Datagrame length: %d", packetLen);
            return err;
        }
        uint32 packetId = *(uint32 *)(&udpBuffer[0]);
        char currSigName[257];
        if(packetId == circuitId)  //The message was for this one
        {
            uint32 currPacketOffs = 16;
            uint16 numPacketSignals = *(uint16*)(&udpBuffer[currPacketOffs]);
            currPacketOffs+=sizeof(uint16);
            samplesReady = false;
            mutex.FastLock();
            for(int sigIdx = 0; sigIdx < numPacketSignals; sigIdx++)
            {
                uint8 sigNameLen = *(uint8 *)&udpBuffer[currPacketOffs];
                currPacketOffs+=sizeof(uint8);
                memcpy(currSigName, &udpBuffer[currPacketOffs], sigNameLen);
                currSigName[sigNameLen] = 0;
                currPacketOffs += sigNameLen;
                uint16 currSampleLen = *(uint16 *)&udpBuffer[currPacketOffs];
                printf("CURR SAMPLE LEN: %d\n", currSampleLen);
                currPacketOffs += sizeof(uint16);
                int32 sigId = getSignalId(currSigName);
                if(sigId == -1)
                {
                    REPORT_ERROR(ErrorManagement::FatalError, "unexpected received signal name: %s", currSigName);
                    mutex.FastUnLock();                    
                    return err;
                }
                if(currSampleLen != sampleByteSizes[sigId])
                {
                    REPORT_ERROR(ErrorManagement::FatalError, "unexpected received signal size for %s: expected: %d received: %d", 
                        currSigName, sampleByteSizes[sigIdx], currSampleLen);
                    mutex.FastUnLock();                    
                    return err;
                }
                printf("%d  %d\n", currSamples[sigId] , samples[sigId]);
                if(currSamples[sigId] < samples[sigId])
                {
                    memcpy(buffer+offsets[sigId]+currSamples[sigId]*sampleByteSizes[sigId], udpBuffer+currPacketOffs, sampleByteSizes[sigId]);
                    currSamples[sigId]++;
                }
                else //shift samples to make room to this
                {
                    for(uint32 sampleIdx = 1; sampleIdx < samples[sigId]; sampleIdx++)
                    {
                        memcpy(buffer+offsets[sigId]+(sampleIdx - 1)*sampleByteSizes[sigId], buffer+offsets[sigId]+sampleIdx*sampleByteSizes[sigId], sampleByteSizes[sigId]);
                    }
                    printf("RICEVO %f (%d)\n", *(float *)(udpBuffer+currPacketOffs), currPacketOffs);
                    printf("OFFSET TOT: %d (%d+%d)\n", offsets[sigId]+(samples[sigId] - 1)*sampleByteSizes[sigId], offsets[sigId], (samples[sigId] - 1)*sampleByteSizes[sigId]);
                    memcpy(buffer+offsets[sigId]+(samples[sigId] - 1)*sampleByteSizes[sigId], udpBuffer+currPacketOffs, sampleByteSizes[sigId]);
               }
               currPacketOffs += sampleByteSizes[sigId];
            }
            //Check if all expected samples have been received
            bool newSamplesReady = true;
            for(uint32 sigIdx = 0; sigIdx < nOfSignals; sigIdx++)
            {
                if(currSamples[sigIdx] < samples[sigIdx])
                {
                    newSamplesReady = false;
                }
            }
            if(!samplesReady && newSamplesReady)
            {
                printf("POST!!!!\n");
                err = !eventSem.Post();
            }
            samplesReady = newSamplesReady;
            mutex.FastUnLock();
        }
    }
    return err;
}

int RTNIn::getSignalId(char8 *sigName)
{
    for (uint32 sigIdx = 0; sigIdx < nOfSignals; sigIdx++)
    {
        if(signalNames[sigIdx] == sigName)
        {
            return sigIdx;
        }
    }
    return -1;
}

CLASS_REGISTER(RTNIn, "1.0")
}

