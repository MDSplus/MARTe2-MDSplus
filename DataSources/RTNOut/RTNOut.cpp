/**
 * @file RTNOut.cpp
 * @brief Source file for class RTNOut
 * @date 25/1/2024
 * @author Gabriele Manduchi
 *

 * @details This source file contains the definition of all the methods for
 * the class RTNOut (public, protected, and private). Be aware that some 
 * methods, such as those inline could be defined on the header file, instead.
 */

/*---------------------------------------------------------------------------*/
/*                         Standard header includes                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                         Project header includes                           */
/*---------------------------------------------------------------------------*/
#include "AdvancedErrorManagement.h"
#include "CLASSMETHODREGISTER.h"
#include "RTNOut.h"
#include <stdio.h>
/*---------------------------------------------------------------------------*/
/*                           Static definitions                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                           Method definitions                              */
/*---------------------------------------------------------------------------*/
namespace MARTe {

RTNOut::RTNOut() :
        DataSourceI() {
    offsets = NULL_PTR(uint32 *);
    dataSourceMemory = NULL_PTR(char8 *);
    signalNames = NULL_PTR(StreamString *);
    sampleByteSizes = NULL_PTR(uint32 *);
    packet = NULL_PTR(char8 *);
	nOfSignals = 0;
    counter = 0;
    maxPacketLen = 0;
    ips = NULL_PTR(StreamString *);
    ports = NULL_PTR(uint32 *);
    circuitIds = NULL_PTR(uint32 *);
    udpSockets = NULL_PTR(UDPSocket *);
    packetLens= NULL_PTR(uint32 *);
    packet = NULL_PTR(char8 *);
        }
/*lint -e{1551} -e{1579} the destructor must guarantee that the memory is freed and the file is flushed and closed.. The brokerAsyncTrigger is freed by the ReferenceT */
RTNOut::~RTNOut() {
    if (offsets != NULL_PTR(uint32 *)) {
        GlobalObjectsDatabase::Instance()->GetStandardHeap()->Free(reinterpret_cast<void *& >(offsets));
    }
    if (dataSourceMemory != NULL_PTR(char8 *)) {
        GlobalObjectsDatabase::Instance()->GetStandardHeap()->Free(reinterpret_cast<void *&>(dataSourceMemory));
    }
    if (signalNames != NULL_PTR(StreamString *)) {
        delete []signalNames;
    }
    if (offsets != NULL_PTR(uint32 *)) {
        GlobalObjectsDatabase::Instance()->GetStandardHeap()->Free(reinterpret_cast<void *& >(sampleByteSizes));
    }
    if (packet != NULL_PTR(char8 *)) {
        GlobalObjectsDatabase::Instance()->GetStandardHeap()->Free(reinterpret_cast<void *&>(packet));
    }
    if(ips != NULL_PTR(StreamString *)) {
        delete[] ips;
    }
    if (ports != NULL_PTR(uint32 *)) {
        GlobalObjectsDatabase::Instance()->GetStandardHeap()->Free(reinterpret_cast<void *& >(ports));
    }
    if (circuitIds != NULL_PTR(uint32 *)) {
        GlobalObjectsDatabase::Instance()->GetStandardHeap()->Free(reinterpret_cast<void *& >(circuitIds));
    }
    if (packetLens != NULL_PTR(uint32 *)) {
        GlobalObjectsDatabase::Instance()->GetStandardHeap()->Free(reinterpret_cast<void *& >(packetLens));
    }
    if(udpSockets != NULL_PTR(UDPSocket *)) {
        delete[] udpSockets;
    }
}

bool RTNOut::AllocateMemory() {
    return true;
}

uint32 RTNOut::GetNumberOfMemoryBuffers() {
    return 1u;
}

bool RTNOut::GetSignalMemoryBuffer(const uint32 signalIdx, const uint32 bufferIdx, void*& signalAddress) {
    bool ok = (dataSourceMemory != NULL_PTR(char8 *));
    if (ok) {
        /*lint -e{613} dataSourceMemory cannot be NULL here*/
        char8 *memPtr = &dataSourceMemory[offsets[signalIdx]];
        signalAddress = reinterpret_cast<void *&>(memPtr);
    }
    return ok;
}

const char8* RTNOut::GetBrokerName(StructuredDataI& data, const SignalDirection direction) {
    const char8* brokerName = "";
    if (direction == OutputSignals) {
            brokerName = "MemoryMapSynchronisedOutputBroker";
    }
    return brokerName;
}

bool RTNOut::GetInputBrokers(ReferenceContainer& inputBrokers, const char8* const functionName, void* const gamMemPtr) {
    return false;
}

bool RTNOut::IsSupportedBroker(const SignalDirection direction, const uint32 functionIdx, const uint32 functionSignalIdx, const char8* const brokerClassName)
{
     return true;
}

bool RTNOut::GetOutputBrokers(ReferenceContainer& outputBrokers, const char8* const functionName, void* const gamMemPtr) 
 { 
     bool ok = true;

    ReferenceT<MemoryMapSynchronisedOutputBroker> broker("MemoryMapSynchronisedOutputBroker");
    ok = broker.IsValid();
    if (ok) {
        ok = broker->Init(OutputSignals, *this, functionName, gamMemPtr);
    }

    if (ok) {
	    ok = outputBrokers.Insert(broker);
    }
   return ok;
}

bool RTNOut::Synchronise() 
{
    for (uint32 sigIdx = 0; sigIdx < nOfSignals; sigIdx++)
    {
        uint32 currOffs = 16;
        memset(packet, 0, packetLens[sigIdx]);
        *(int32 *)&packet[0] = circuitIds[sigIdx];
        *(int32 *)&packet[4] = counter;

        *(int16 *)&packet[currOffs] = 1; //1 signal per packet
        currOffs += 2;
        *(int8 *)&packet[currOffs] = signalNames[sigIdx].Size();
        currOffs++;
        memcpy(&packet[currOffs], signalNames[sigIdx].Buffer(), signalNames[sigIdx].Size());
        currOffs += signalNames[sigIdx].Size();
        *(int16 *)(&packet[currOffs]) = sampleByteSizes[sigIdx];
        currOffs += sizeof(int16); 
        printf("SPEDISCO %f\n", *(float *)&dataSourceMemory[offsets[sigIdx]]);
        memcpy(&packet[currOffs], &dataSourceMemory[offsets[sigIdx]], sampleByteSizes[sigIdx]);
        if(!udpSockets[sigIdx].Write(packet, packetLens[sigIdx]))
        {
            REPORT_ERROR(ErrorManagement::FatalError,"Error sending UDP packet");
            return false;
        }
    }
    return true;
}
 
 
 
 
/*lint -e{715}  [MISRA C++ Rule 0-1-11], [MISRA C++ Rule 0-1-12]. Justification: NOOP at StateChange, independently of the function parameters.*/
bool RTNOut::PrepareNextState(const char8* const currentStateName, const char8* const nextStateName) {
    return true;
}



bool RTNOut::Initialise(StructuredDataI& data) {
    bool ok = DataSourceI::Initialise(data);
    if(ok)
    {
        ok = data.MoveRelative("Signals");
        if(!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError,"Signals node Missing.");
        }
    }
    if(ok)
    {
        nOfSignals = data.GetNumberOfChildren();
        ips = new StreamString[nOfSignals];
        ports = reinterpret_cast<uint32 *>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(nOfSignals * sizeof(int32)));
        circuitIds = reinterpret_cast<uint32 *>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(nOfSignals * sizeof(int32)));
        for (uint32 sigIdx = 0u; (sigIdx < nOfSignals) && ok; sigIdx++) {
            ok = data.MoveRelative(data.GetChildName(sigIdx));
            if (!ok) {
                REPORT_ERROR(ErrorManagement::ParametersError, "Cannot move to the children %u", sigIdx);
            }
            if (ok) {
                ok = data.Read("Ip", ips[sigIdx]);
                if (!ok) {
                    REPORT_ERROR(ErrorManagement::ParametersError, "Ip shall be specified for signal %d", sigIdx);
                }
            }
            if (ok) {
                ok = data.Read("Port", ports[sigIdx]);
                if (!ok) {
                    REPORT_ERROR(ErrorManagement::ParametersError, "Port shall be specified for signal %d", sigIdx);
                }
            }
            if (ok) {
                ok = data.Read("CircuitId", circuitIds[sigIdx]);
                if (!ok) {
                    REPORT_ERROR(ErrorManagement::ParametersError, "CircuitId shall be specified");
                }
            }
            data.MoveToAncestor(1u);
        }
        data.MoveToAncestor(1u);
    }
    return ok;
}


bool RTNOut::SetConfiguredDatabase(StructuredDataI& data) {
    uint32 totalSignalMemory = 0;
    bool ok = DataSourceI::SetConfiguredDatabase(data);
    //Check signal properties and compute memory
    
    if (ok) { // Check that only one GAM is Connected to the MDSReaderNS
        uint32 auxNumberOfFunctions = GetNumberOfFunctions();
        ok = (auxNumberOfFunctions == 1u);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "Exactly one Function allowed to interact with this RTNOut DataSource. Number of Functions = %u",
                         auxNumberOfFunctions);
        }
    }
    if (ok) {
        for (uint32 n = 0u; (n < nOfSignals) && ok; n++) {
            uint32 nSamples;
            ok = GetFunctionSignalSamples(OutputSignals, 0u, n, nSamples);
            if (ok) {
                ok = (nSamples == 1u);
            }
            if (!ok) {
                REPORT_ERROR(ErrorManagement::ParametersError, "The number of samples shall be exactly 1");
            }
        }
    }
    if (ok) { //Count and allocate memory for dataSourceMemory, lastValue and lastTime
	//Count the number of bytes
        udpSockets = new UDPSocket[nOfSignals];
        packetLens = reinterpret_cast<uint32 *>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(nOfSignals * sizeof(int32)));
        signalNames = new StreamString[nOfSignals];
       	offsets  = reinterpret_cast<uint32 *>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(nOfSignals * sizeof(int32)));
      	sampleByteSizes  = reinterpret_cast<uint32 *>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(nOfSignals * sizeof(int32)));
    	memset(offsets, 0, nOfSignals * sizeof(int32));
        maxPacketLen = 0;
        totalSignalMemory = 0;
        for (uint32 sigIdx = 0u; (sigIdx < nOfSignals) && ok; sigIdx++) {
            uint32 nBytes;
            packetLens[sigIdx] = 16 + 2;
		    offsets[sigIdx] = totalSignalMemory;
		    ok = GetSignalByteSize(sigIdx, nBytes);
		    if (!ok) {
                        REPORT_ERROR(ErrorManagement::ParametersError, "Error while GetSignalByteSize() for signal %u", sigIdx);
		    }
		    ok = GetSignalName(sigIdx, signalNames[sigIdx]);
		    if (!ok) {
                        REPORT_ERROR(ErrorManagement::ParametersError, "Error while GetSignalName() for signal %u", sigIdx);
		    }
            packetLens[sigIdx] += 1 + signalNames[sigIdx].Size() +2 + nBytes;
		    totalSignalMemory += nBytes;
            sampleByteSizes[sigIdx] = nBytes;
            if (packetLens[sigIdx] > maxPacketLen)
                maxPacketLen = packetLens[sigIdx];
        }
    }
    if(ok)
    {
      	dataSourceMemory = reinterpret_cast<char8*>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(totalSignalMemory));
       	packet = reinterpret_cast<char8*>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(maxPacketLen));
    }
    if(ok)
    {
        for (uint32 sigIdx = 0u; (sigIdx < nOfSignals) && ok; sigIdx++) {
   	        ok =udpSockets[sigIdx].Open();
            if(!ok)
            {
                REPORT_ERROR(ErrorManagement::ParametersError, "Cannot open socket for signal %d", sigIdx);
            }
        }
    }
    if(ok)
    {
        for (uint32 sigIdx = 0u; (sigIdx < nOfSignals) && ok; sigIdx++) {
            do {
                printf("Connecting to %s   %d....\n", ips[sigIdx].Buffer(), ports[sigIdx]);
                ok = udpSockets[sigIdx].Connect(ips[sigIdx].Buffer(), ports[sigIdx]);
                if(!ok)
                {
                    sleep(1);
                }
            }while(!ok);
            printf("Connected!\n");
            if(!ok)
            {
                REPORT_ERROR(ErrorManagement::ParametersError, "Cannot Connect to %s", ips[sigIdx].Buffer());
            }
        }
       
    }
    return ok;
}




uint32 RTNOut::GetNumberOfBuffers() const {
    return 1;
}


CLASS_REGISTER(RTNOut, "1.0")
}

