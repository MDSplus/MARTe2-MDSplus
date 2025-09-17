/**
 * @file EEIOut.cpp
 * @brief Source file for class EEIOut
 * @date 25/1/2024
 * @author Gabriele Manduchi
 *

 * @details This source file contains the definition of all the methods for
 * the class EEIOut (public, protected, and private). Be aware that some 
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
#include "EEIOut.h"
#include <stdio.h>
/*---------------------------------------------------------------------------*/
/*                           Static definitions                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                           Method definitions                              */
/*---------------------------------------------------------------------------*/

//#define DEBUG 1
namespace MARTe {

EEIOut::EEIOut() : DataSourceI() {
    offsets = NULL_PTR(uint32 *);
    dataSourceMemory = NULL_PTR(char8 *);
    packet = NULL_PTR(char8 *);
	nOfSignals = 0;
    counter = 0;
    packetLen = 0;
    circuitId = 0;
}
/*lint -e{1551} -e{1579} the destructor must guarantee that the memory is freed and the file is flushed and closed.. The brokerAsyncTrigger is freed by the ReferenceT */
EEIOut::~EEIOut() {
    if (offsets != NULL_PTR(uint32 *)) {
        delete [] offsets;
    }
    if (dataSourceMemory != NULL_PTR(char8 *)) {
        delete [] dataSourceMemory;
    }
    if (packet != NULL_PTR(char8 *)) {
        delete [] packet;
    }
}

bool EEIOut::AllocateMemory() {
    return true;
}

uint32 EEIOut::GetNumberOfMemoryBuffers() {
    return 1u;
}

bool EEIOut::GetSignalMemoryBuffer(const uint32 signalIdx, const uint32 bufferIdx, void*& signalAddress) {
    bool ok = (dataSourceMemory != NULL_PTR(char8 *));
    if (ok) {
        /*lint -e{613} dataSourceMemory cannot be NULL here*/
        char8 *memPtr = &dataSourceMemory[offsets[signalIdx]];
        signalAddress = reinterpret_cast<void *&>(memPtr);
    }
    return ok;
}

const char8* EEIOut::GetBrokerName(StructuredDataI& data, const SignalDirection direction) {
    const char8* brokerName = "";
    if (direction == OutputSignals) {
            brokerName = "MemoryMapSynchronisedOutputBroker";
    }
    return brokerName;
}

bool EEIOut::GetInputBrokers(ReferenceContainer& inputBrokers, const char8* const functionName, void* const gamMemPtr) {
    return false;
}

bool EEIOut::IsSupportedBroker(const SignalDirection direction, const uint32 functionIdx, const uint32 functionSignalIdx, const char8* const brokerClassName)
{
     return true;
}

bool EEIOut::GetOutputBrokers(ReferenceContainer& outputBrokers, const char8* const functionName, void* const gamMemPtr) 
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

bool EEIOut::Synchronise() 
{
    memset(packet, 0, HEADER_LEN);
    memcpy(&packet[0], &circuitId, sizeof(uint32));
    memcpy(&packet[sizeof(int32)], &counter, sizeof(uint32));
    memcpy(&packet[2*sizeof(int32)], dataSourceMemory, sizeof(uint32));
    memcpy(&packet[HEADER_LEN], &dataSourceMemory[sizeof(int32)], dataSourceMemoryLen - sizeof(int32));
    uint32 packetLen = HEADER_LEN + dataSourceMemoryLen - sizeof(int32);
#ifdef DEBUG
    printf("Sent Counter: %d Time: %d\n", counter, *(uint32 *)dataSourceMemory);
#endif
    if(!udpSocket.Write(packet, packetLen))
    {
        REPORT_ERROR(ErrorManagement::FatalError,"Error sending UDP packet");
        return false;        
    }
    counter++;
    return true;
}
 
 
/*lint -e{715}  [MISRA C++ Rule 0-1-11], [MISRA C++ Rule 0-1-12]. Justification: NOOP at StateChange, independently of the function parameters.*/
bool EEIOut::PrepareNextState(const char8* const currentStateName, const char8* const nextStateName) {
    return true;
}



bool EEIOut::Initialise(StructuredDataI& data) {
    bool ok = DataSourceI::Initialise(data);
    //StreamString *ips;
    //uint32 *ports;
    if(ok)
    {
        nOfSignals = data.GetNumberOfChildren();
        if(nOfSignals < 1)
        {
            REPORT_ERROR(ErrorManagement::ParametersError, "At least One signal (Time) shall be specified.");
            ok = false;
        }
     }
    if(ok)
    {
        ok = data.Read("Ip", ip);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "Ip shall be specified.");
        }
    }
    if (ok) {
        ok = data.Read("Port", port);
        if(!ok)
            REPORT_ERROR(ErrorManagement::ParametersError, "Port shall be specified");
    }
    if (ok) 
    {
        ok = data.Read("CircuitId", circuitId);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "CircuitId shall be specified");
        }
    }
    if(ok)
    {
        ok = udpSocket.Open();
        if(!ok)
        {
            REPORT_ERROR(ErrorManagement::ParametersError, "Cannot open socket.");
        }
    }
    if(ok)
    {
       printf("Connecting to %s   %d....\n", ip.Buffer(), port);
        ok = udpSocket.Connect(ip.Buffer(), port);
        if(!ok)
        {
            REPORT_ERROR(ErrorManagement::ParametersError, "Cannot Connect to %s", ip.Buffer());
        }
    }
    return ok;
}


bool EEIOut::SetConfiguredDatabase(StructuredDataI& data) {
    bool ok = DataSourceI::SetConfiguredDatabase(data);
    //Check signal properties and compute memory
    
    if (ok) { // Check that only one GAM is Connected to the MDSReaderNS
        uint32 auxNumberOfFunctions = GetNumberOfFunctions();
        ok = (auxNumberOfFunctions == 1u);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "Exactly one Function allowed to interact with this EEIOut DataSource. Number of Functions = %u",
                         auxNumberOfFunctions);
        }
    }
    nOfSignals = GetNumberOfSignals();
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
    StreamString currSignalName;
    GetSignalName(0, currSignalName);
    if(currSignalName != "Time")
    {
        REPORT_ERROR(ErrorManagement::ParametersError, "The fist signal shall be named Time");
        return false;

    }
    ok = GetSignalType(0) == SignedInteger32Bit || (GetSignalType(0) == UnsignedInteger32Bit);
    if(!ok)
    {
        REPORT_ERROR(ErrorManagement::ParametersError, "Type of Time shall be either int32 or uint32");
        return false;
    }
    offsets  = new uint32[nOfSignals];
    dataSourceMemoryLen = 0;
    for (uint32 sigIdx = 0u; (sigIdx < nOfSignals) && ok; sigIdx++) {
        uint32 nBytes;
        offsets[sigIdx] = dataSourceMemoryLen;
    	ok = GetSignalByteSize(sigIdx, nBytes);
		if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "Error while GetSignalByteSize() for signal %u", sigIdx);
		}
        dataSourceMemoryLen += nBytes;
    }
    if(ok)
    {
      	dataSourceMemory = new char8[dataSourceMemoryLen];
       	packet = new char8[HEADER_LEN + dataSourceMemoryLen - sizeof(int32)];  //Time is reported in header
    }
    return ok;
}




uint32 EEIOut::GetNumberOfBuffers() const {
    return 1;
}


CLASS_REGISTER(EEIOut, "1.0")
}

