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
#include "StructuredDataIHelper.h"
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
    offsets = NULL_PTR(uint32 **);
    dataSourceMemory = NULL_PTR(char8 *);
    signalNames = NULL_PTR(StreamString **);
    sampleByteSizes = NULL_PTR(uint32 **);
    nOfSignalFields = NULL_PTR(uint32 *);
    packet = NULL_PTR(char8 *);
	nOfSignals = 0;
    counter = 0;
    maxPacketLen = 0;
    circuitIds = NULL_PTR(uint32 **);
    udpSockets = NULL_PTR(UDPSocket **);
    packetLens= NULL_PTR(uint32 *);
    packet = NULL_PTR(char8 *);
        }
/*lint -e{1551} -e{1579} the destructor must guarantee that the memory is freed and the file is flushed and closed.. The brokerAsyncTrigger is freed by the ReferenceT */
RTNOut::~RTNOut() {
    if (offsets != NULL_PTR(uint32 **)) {
        for(uint32 i = 0; i < nOfSignalFields[i]; i++)
            delete [] offsets[i];
        delete [] offsets;
    }
    if (sampleByteSizes != NULL_PTR(uint32 **)) {
        for(uint32 i = 0; i < nOfSignalFields[i]; i++)
            delete [] sampleByteSizes[i];
        delete [] sampleByteSizes;
    }
    if (nOfSignalFields != NULL_PTR(uint32 *)) {
         delete [] nOfSignalFields;
    }
    if (dataSourceMemory != NULL_PTR(char8 *)) {
        delete [] dataSourceMemory;
    }
    if (signalNames != NULL_PTR(StreamString **)) {
        for(uint32 i = 0; i < nOfSignalFields[i]; i++)
            delete [] signalNames[i];
       delete []signalNames;
    }
    if (packet != NULL_PTR(char8 *)) {
        delete [] packet;
    }
    if (circuitIds != NULL_PTR(uint32 **)) {
        for (uint32 i = 0; i < nOfSignals; i++)
        {
            delete [] circuitIds[i];
        }
        delete []circuitIds;
    }
    if (packetLens != NULL_PTR(uint32 *)) {
       delete []packetLens;
    }
    if(udpSockets != NULL_PTR(UDPSocket **)) {
       for (uint32 i = 0; i < nOfSignals; i++)
        {
            delete [] udpSockets[i];
        } 
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
        uint32 sigIdx = 0, currIdx = 0, offsetIdx = 0;
//Retrieve corresponding offset in structure
        while(true)
        {
            currIdx += nOfSignalFields[sigIdx];
            if(currIdx > signalIdx)
            {
                currIdx -= nOfSignalFields[sigIdx];
                offsetIdx = signalIdx - currIdx;
                break;
            }
            sigIdx++;
        }

        char8 *memPtr = &dataSourceMemory[offsets[sigIdx][offsetIdx]];

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
        *(int32 *)&packet[4] = counter;

        *(int16 *)&packet[currOffs] = nOfSignalFields[sigIdx]; //1 signal per packet if not structured, otherwise as many as the fields
        currOffs += 2;
        for(uint32 fieldIdx = 0; fieldIdx < nOfSignalFields[sigIdx]; fieldIdx++)
        {
            *(int8 *)&packet[currOffs] = signalNames[sigIdx][fieldIdx].Size();
            currOffs++;
            memcpy(&packet[currOffs], signalNames[sigIdx][fieldIdx].Buffer(), signalNames[sigIdx][fieldIdx].Size());
            currOffs += signalNames[sigIdx][fieldIdx].Size();
            *(int16 *)(&packet[currOffs]) = sampleByteSizes[sigIdx][fieldIdx];
            currOffs += sizeof(int16); 
//        printf("SPEDISCO %f\n", *(float *)&dataSourceMemory[offsets[sigIdx]]);
            memcpy(&packet[currOffs], &dataSourceMemory[offsets[sigIdx][fieldIdx]], sampleByteSizes[sigIdx][fieldIdx]);
            currOffs += sampleByteSizes[sigIdx][fieldIdx];
        }
        for (uint32 currIdx = 0; currIdx < numIps[sigIdx]; currIdx++)
        { 
            *(int32 *)&packet[0] = circuitIds[sigIdx][currIdx];
            if(!udpSockets[sigIdx][currIdx].Write(packet, packetLens[sigIdx]))
            {
                REPORT_ERROR(ErrorManagement::FatalError,"Error sending UDP packet");
                return false;
            }
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
    StructuredDataIHelper helper(data, this);
    StreamString *ips;
    uint32 *ports;
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
        numIps = new uint32[nOfSignals];
        udpSockets = new UDPSocket *[nOfSignals];
        nOfSignalFields = new uint32 [nOfSignals];
        circuitIds = new uint32 *[nOfSignals];
        for (uint32 sigIdx = 0u; (sigIdx < nOfSignals) && ok; sigIdx++) {
            ok = data.MoveRelative(data.GetChildName(sigIdx));
            if (!ok) {
                REPORT_ERROR(ErrorManagement::ParametersError, "Cannot move to the children %u", sigIdx);
            }
            if (ok) {
                ok = helper.ReadArray("Ip", ips, numIps[sigIdx]);
                if (!ok) {
                    ips = new StreamString[1];
                    numIps[sigIdx] = 1;
                    ok = data.Read("Ip", ips[0]);
                    if(!ok)
                        REPORT_ERROR(ErrorManagement::ParametersError, "Ips array shall be specified for signal %d", sigIdx);
                }
            }

            if (ok) {
                uint32 numPorts;
               ok =helper.ReadArray("Port", ports, numPorts);
                if (!ok) {
                    ports = reinterpret_cast<uint32 *>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(sizeof(int32)));
                    numPorts = 1;
                    ok = data.Read("Port", ports[0]);
                    if(!ok)
                        REPORT_ERROR(ErrorManagement::ParametersError, "Ips array shall be specified for signal %d", sigIdx);
                }
                if(ok)
                {
                    if(numPorts != numIps[sigIdx])
                    {
                        REPORT_ERROR(ErrorManagement::ParametersError, "Dimension of Ports shall be the same of Ips for signal %d", sigIdx);
                        ok = false;
                    }
                }
            }
            if (ok) 
            {
                uint32 numCircuitIds;
                ok = helper.ReadArray("CircuitId", circuitIds[sigIdx], numCircuitIds);
                if(!ok)
                {
                    numCircuitIds = 1;
                    circuitIds[sigIdx] = reinterpret_cast<uint32 *>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(sizeof(int32)));
                    ok = data.Read("CircuitId", circuitIds[sigIdx][0]);
                }
                if (!ok) {
                    REPORT_ERROR(ErrorManagement::ParametersError, "CircuitId shall be specified");
                }
                else
                {
                    if(numCircuitIds != numIps[sigIdx])
                    {
                        REPORT_ERROR(ErrorManagement::ParametersError, "Dimension of CircuitId shall be the same of Ips for signal %d", sigIdx);
                        ok = false;
                    }
                }
            }
            if(ok)
            {
                udpSockets[sigIdx] = new UDPSocket[numIps[sigIdx]];
                for(uint32 currIdx = 0; ok && (currIdx < numIps[sigIdx]); currIdx++)
                {
                    ok = udpSockets[sigIdx][currIdx].Open();
                    if(!ok)
                    {
                        REPORT_ERROR(ErrorManagement::ParametersError, "Cannot open socket for signal %d", sigIdx);
                    }
                }
            }
            if(ok)
            {
                for(uint32 currIdx = 0; ok && (currIdx < numIps[sigIdx]); currIdx++)
                {
                    printf("Connecting to %s   %d....\n", ips[currIdx].Buffer(), ports[currIdx]);
                    ok = udpSockets[sigIdx][currIdx].Connect(ips[currIdx].Buffer(), ports[currIdx]);
                    if(!ok)
                    {
                        REPORT_ERROR(ErrorManagement::ParametersError, "Cannot Connect to %s", ips[sigIdx].Buffer());
                    }
                } 
                delete [] ips;
                GlobalObjectsDatabase::Instance()->GetStandardHeap()->Free(reinterpret_cast<void *&>(ports));               
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
        packetLens = new uint32[nOfSignals];
        signalNames = new StreamString *[nOfSignals];
       	offsets  = new uint32*[nOfSignals];
      	sampleByteSizes  = new uint32 *[nOfSignals];
    	memset(offsets, 0, nOfSignals * sizeof(int32));
        maxPacketLen = 0;
        totalSignalMemory = 0;
        uint32 actSigIdx = 0;
        uint32 nSamples;
        uint32 actNumSignals = GetNumberOfSignals();
        for (uint32 sigIdx = 0u; (sigIdx < nOfSignals) && ok; sigIdx++) {
            uint32 nBytes;
            packetLens[sigIdx] = 16 + 2;
            StreamString currSignalName;
            ok = GetSignalName(actSigIdx, currSignalName);
            int32 idx;
            if((idx = currSignalName.Locate('.')) != -1) //If it refers to a field of a structuee 
            {
                nOfSignalFields[sigIdx] = 0;
                uint32 currSigIdx = actSigIdx;
                while(currSigIdx < actNumSignals)
                {
                    StreamString currFieldName;
                    ok = GetSignalName(currSigIdx, currFieldName);
                    if (!ok) {
                        REPORT_ERROR(ErrorManagement::ParametersError, "Error while GetSignalName() for signal %u", sigIdx);
                        return false;
                    }
                    if(strncmp(currSignalName.Buffer(), currFieldName.Buffer(), idx + 1)) //If the first part, including dot are differents
                        break;
                    currSigIdx++;
                    nOfSignalFields[sigIdx]++;
                }
                currSigIdx = actSigIdx;
                signalNames[sigIdx] = new StreamString[nOfSignalFields[sigIdx]];
                offsets[sigIdx] = new uint32[nOfSignalFields[sigIdx]];
                sampleByteSizes[sigIdx] = new uint32[nOfSignalFields[sigIdx]];
                for(uint32 i = 0; i < nOfSignalFields[sigIdx]; i++)
                {
                    GetSignalName(currSigIdx, signalNames[sigIdx][i]);
                    offsets[sigIdx][i] = totalSignalMemory;
		            ok = GetSignalByteSize(currSigIdx, nBytes);
		            if (!ok) {
                        REPORT_ERROR(ErrorManagement::ParametersError, "Error while GetSignalByteSize() for signal %u", sigIdx);
		            }
                    sampleByteSizes[sigIdx][i] = nBytes;
                    totalSignalMemory += nBytes;
                    packetLens[sigIdx] += 1 + signalNames[sigIdx][i].Size() +2 + nBytes;
                    if (packetLens[sigIdx] > maxPacketLen)
                        maxPacketLen = packetLens[sigIdx];
                    ok = GetFunctionSignalSamples(OutputSignals, 0u, currSigIdx, nSamples);
                    if (ok) {
                        ok = (nSamples == 1u);
                    }
                    if (!ok) {
                        REPORT_ERROR(ErrorManagement::ParametersError, "The number of samples shall be exactly 1");
                    }
                    currSigIdx++;
                }
                actSigIdx = currSigIdx;
            }
            else //Not a structured type
            {
                nOfSignalFields[sigIdx] = 1;
                signalNames[sigIdx] = new StreamString[1];
                sampleByteSizes[sigIdx] = new uint32[1];
                offsets[sigIdx] = new uint32[1];
                offsets[sigIdx][0] = totalSignalMemory;
                signalNames[sigIdx][0] = currSignalName;
                ok = GetSignalByteSize(actSigIdx, nBytes);
                if (!ok) {
                    REPORT_ERROR(ErrorManagement::ParametersError, "Error while GetSignalByteSize() for signal %u", sigIdx);
                }
                sampleByteSizes[sigIdx][0] = nBytes;
                totalSignalMemory += nBytes;
                packetLens[sigIdx] += 1 + signalNames[sigIdx][0].Size() +2 + nBytes;
                if (packetLens[sigIdx] > maxPacketLen)
                    maxPacketLen = packetLens[sigIdx];
                ok = GetFunctionSignalSamples(OutputSignals, 0u, actSigIdx, nSamples);
                if (ok) {
                    ok = (nSamples == 1u);
                }
                if (!ok) {
                    REPORT_ERROR(ErrorManagement::ParametersError, "The number of samples shall be exactly 1");
                }
               actSigIdx++; //Count just the signal
            }
        }
        if(ok && (actSigIdx != actNumSignals))
        {
            ok = false;
            REPORT_ERROR(ErrorManagement::ParametersError, "Internal error actNumSignals(%d) different from computed Num Signals (%d)", actNumSignals, actSigIdx);
        }
    }
    if(ok)
    {
      	dataSourceMemory = new char8[totalSignalMemory];
       	packet = new char8[maxPacketLen];
    }
    return ok;
}




uint32 RTNOut::GetNumberOfBuffers() const {
    return 1;
}


CLASS_REGISTER(RTNOut, "1.0")
}

