/**
 * @file ConsoleOut.cpp
 * @brief Source file for class ConsoleOut
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

 * @details This source file contains the definition of all the methods for
 * the class ConsoleOut (public, protected, and private). Be aware that some 
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
#include "TcpOut.h"
#include <stdio.h>
/*---------------------------------------------------------------------------*/
/*                           Static definitions                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                           Method definitions                              */
/*---------------------------------------------------------------------------*/
namespace MARTe {

TcpOut::TcpOut() :
        DataSourceI(),
        MessageI() {
	dataSourceMemory = NULL_PTR(char8 *);
	offsets = NULL_PTR(uint32 *);
	nOfSignals = 0;
}

/*lint -e{1551} -e{1579} the destructor must guarantee that the memory is freed and the file is flushed and closed.. The brokerAsyncTrigger is freed by the ReferenceT */
TcpOut::~TcpOut() {
    if (dataSourceMemory != NULL_PTR(char8 *)) {
        GlobalObjectsDatabase::Instance()->GetStandardHeap()->Free(reinterpret_cast<void *&>(dataSourceMemory));
    }
    if (offsets != NULL_PTR(uint32 *)) {
        GlobalObjectsDatabase::Instance()->GetStandardHeap()->Free(reinterpret_cast<void *& >(offsets));
    }
}

bool TcpOut::AllocateMemory() {
    return true;
}

uint32 TcpOut::GetNumberOfMemoryBuffers() {
    return 1u;
}

bool TcpOut::GetSignalMemoryBuffer(const uint32 signalIdx, const uint32 bufferIdx, void*& signalAddress) {
    bool ok = (dataSourceMemory != NULL_PTR(char8 *));
    if (ok) {
        /*lint -e{613} dataSourceMemory cannot be NULL here*/
        char8 *memPtr = &dataSourceMemory[offsets[signalIdx]];
        signalAddress = reinterpret_cast<void *&>(memPtr);
    }
    return ok;
}

const char8* TcpOut::GetBrokerName(StructuredDataI& data, const SignalDirection direction) {
    const char8* brokerName = "";
    if (direction == OutputSignals) {
            brokerName = "MemoryMapSynchronisedOutputBroker";
    }
    return brokerName;
}

bool TcpOut::GetInputBrokers(ReferenceContainer& inputBrokers, const char8* const functionName, void* const gamMemPtr) {
    return false;
}

bool TcpOut::IsSupportedBroker(const SignalDirection direction, const uint32 functionIdx, const uint32 functionSignalIdx, const char8* const brokerClassName)
{
     return true;
}

bool TcpOut::GetOutputBrokers(ReferenceContainer& outputBrokers, const char8* const functionName, void* const gamMemPtr) 
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

bool TcpOut::Synchronise() 
{
    for (uint32 sigIdx = 0; sigIdx < nOfSignals; sigIdx++)
    {
        uint32 size = sizeof(float64);
        if(!socket.Write(dataSourceMemory + offsets[sigIdx], size))
        {
            REPORT_ERROR(ErrorManagement::FatalError,"Error sending TCP data");
            return false;
        }
    }
    return true;
}
 
 
 
 
/*lint -e{715}  [MISRA C++ Rule 0-1-11], [MISRA C++ Rule 0-1-12]. Justification: NOOP at StateChange, independently of the function parameters.*/
bool TcpOut::PrepareNextState(const char8* const currentStateName, const char8* const nextStateName) {
    return true;
}



bool TcpOut::Initialise(StructuredDataI& data) {
    bool ok = DataSourceI::Initialise(data);
    if (ok) {
        ok = data.Read("Ip", ip);
    }
    if (!ok) {
        REPORT_ERROR(ErrorManagement::ParametersError, "Ip");
    }
    if (ok) {
        ok = data.Read("Port", port);
    }
    if (!ok) {
        REPORT_ERROR(ErrorManagement::ParametersError, "Port shall be specified");
    }
    ok = data.MoveRelative("Signals");
    if(!ok) {
	    REPORT_ERROR(ErrorManagement::ParametersError,"Signals node Missing.");
	    return ok;
    }
    nOfSignals = data.GetNumberOfChildren();
    data.MoveToAncestor(1u);
    return ok;
}

bool TcpOut::SetConfiguredDatabase(StructuredDataI& data) {
    bool ok = DataSourceI::SetConfiguredDatabase(data);
    //Check signal properties and compute memory
    
    if (ok) { // Check that only one GAM is Connected to the MDSReaderNS
        uint32 auxNumberOfFunctions = GetNumberOfFunctions();
        ok = (auxNumberOfFunctions == 1u);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "Exactly one Function allowed to interact with this TcpOut DataSource. Number of Functions = %u",
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
            if(ok)
            {
	            TypeDescriptor currType = GetSignalType(n);
	            if ((currType != Float64Bit))
 	            {
           	        REPORT_ERROR(ErrorManagement::ParametersError, "Signals shall be Float64");
	    	        ok = false;
 	            }
            }
        }
    }
    if (ok) { //Count and allocate memory for dataSourceMemory, lastValue and lastTime
        offsets = new uint32[nOfSignals];
	//Count the number of bytes
        totalSignalMemory = 0u;
        uint32 nBytes = 0;
       	offsets  = reinterpret_cast<uint32 *>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(nOfSignals * sizeof(int32)));
    	memset(offsets, 0, nOfSignals * sizeof(int32));
        for (uint32 i = 0u; (i < nOfSignals) && ok; i++) {
		      offsets[i] = totalSignalMemory;
		      ok = GetSignalByteSize(i, nBytes);
		      if (!ok) {
                        REPORT_ERROR(ErrorManagement::ParametersError, "Error while GetSignalByteSize() for signal %u", i);
		      }
		      totalSignalMemory += nBytes;
        }
    }
    if(ok)
    {
      	dataSourceMemory = reinterpret_cast<char8*>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(totalSignalMemory));
    }
    if(ok)
    {
   	    ok =socket.Open();
        if(!ok)
        {
            REPORT_ERROR(ErrorManagement::ParametersError, "Cannot open socket");
        }
    }
    if(ok)
    {
        do {
            printf("Connecting....\n");
   	        ok = socket.Connect(ip.Buffer(), port);
            if(!ok)
            {
                sleep(1);
            }
        }while(!ok);
        printf("Connected!\n");
        if(!ok)
        {
            REPORT_ERROR(ErrorManagement::ParametersError, "Cannot Connect socket");
        }
       
    }
    return ok;
}




uint32 TcpOut::GetNumberOfBuffers() const {
    return 1;
}


CLASS_REGISTER(TcpOut, "1.0")
}

