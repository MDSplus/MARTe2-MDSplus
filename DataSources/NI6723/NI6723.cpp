/**
 * @file NI6723.cpp
 * @brief Source file for class NI6723
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
 * the class NI6723 (public, protected, and private). Be aware that some 
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
#include "NI6723.h"
#include <mdsobjects.h>
#include <stdio.h>
/*---------------------------------------------------------------------------*/
/*                           Static definitions                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                           Method definitions                              */
/*---------------------------------------------------------------------------*/
namespace MARTe {
NI6723::NI6723() :
        DataSourceI(),
        MessageI() {
	  numOutChannels = 0;


}

/*lint -e{1551} -e{1579} the destructor must guarantee that the memory is freed and the file is flushed and closed.. The brokerAsyncTrigger is freed by the ReferenceT */
NI6723::~NI6723() {
}

bool NI6723::AllocateMemory() {
    return true;
}

uint32 NI6723::GetNumberOfMemoryBuffers() {
    return 1u;
}

bool NI6723::GetSignalMemoryBuffer(const uint32 signalIdx, const uint32 bufferIdx, void*& signalAddress) {
    signalAddress = static_cast<void *>(&outBuffers[signalIdx]);
    return true;
}

const char8* NI6723::GetBrokerName(StructuredDataI& data, const SignalDirection direction) {
    const char8* brokerName = "";
    if (direction == OutputSignals) {
            brokerName = "MemoryMapSynchronisedOutputBroker";
    }
    return brokerName;
}

bool NI6723::GetInputBrokers(ReferenceContainer& inputBrokers, const char8* const functionName, void* const gamMemPtr) {
    return false;
}

bool NI6723::IsSupportedBroker(const SignalDirection direction, const uint32 functionIdx, const uint32 functionSignalIdx, const char8* const brokerClassName)
{
     return true;
}

bool NI6723::GetOutputBrokers(ReferenceContainer& outputBrokers, const char8* const functionName, void* const gamMemPtr) {
  
    bool ok = true;
//    ReferenceT<MemoryMapSynchronisedOutputBroker> brokerSynch("MemoryMapSynchronisedOutputBroker");
    ReferenceT<MemoryMapSynchronisedOutputBroker> brokerSynch("MemoryMapSynchronisedOutputBroker");
    ok = brokerSynch.IsValid();
    if (ok) {
	ok = brokerSynch->Init(OutputSignals, *this, functionName, gamMemPtr);
    }
    if (ok) {
	ok = outputBrokers.Insert(brokerSynch);
    }
   return ok;
}

bool NI6723::Synchronise() {
    for(size_t i = 0; i < numOutChannels; i++)
    {
	if(outChannels[i] > 4095)
	    outBuffers[i] = 4095;
	else if (outChannels[i] < -4096)
 	    outBuffers[i] = -4096;
	else
	    outBuffers[i] = outChannels[i];
    }
    module.Write((u32 *)outBuffers, numOutChannels);
    return true;
}
 
 
 
 
/*lint -e{715}  [MISRA C++ Rule 0-1-11], [MISRA C++ Rule 0-1-12]. Justification: NOOP at StateChange, independently of the function parameters.*/
bool NI6723::PrepareNextState(const char8* const currentStateName, const char8* const nextStateName) {
    return true;
}

bool NI6723::Initialise(StructuredDataI& data) {
    bool ok = DataSourceI::Initialise(data);
    ok = data.Read("Address", address);
    if(!ok)
    {
	REPORT_ERROR(ErrorManagement::ParametersError,"NI6723 device Address shall be specified");
	return ok;
    }
    return ok;
}

bool NI6723::SetConfiguredDatabase(StructuredDataI& data) {
    bool ok = DataSourceI::SetConfiguredDatabase(data);
    //Check signal properties and compute memory
    
    if (ok) { // Check that only one GAM is Connected to the NI6723
        uint32 auxNumberOfFunctions = GetNumberOfFunctions();
        ok = (auxNumberOfFunctions == 1u);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "Exactly one Function allowed to interact with this NI6723 DataSource. Number of Functions = %u",
                         auxNumberOfFunctions);
	    return false;
        }
    }

    numOutChannels = GetNumberOfSignals();
    ok = (numOutChannels > 0u && numOutChannels <= 80);
    if (!ok) {
        REPORT_ERROR(ErrorManagement::ParametersError, "NumberOfOutputs shall be between 1 and 80");
	return false;
    }
    for(uint32 sigIdx = 0; sigIdx < numOutChannels; sigIdx++)
    {
        if(ok)
        {
	    TypeDescriptor sigType = GetSignalType(sigIdx);
	    ok = (sigType == SignedInteger16Bit);
            if (!ok) {
                REPORT_ERROR(ErrorManagement::ParametersError, "Signal type shall be Int16");
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
    if(ok)
    {
	int status = module.Init((char *)address.Buffer());
	ok = (status >= 0);
	if(!ok) {
                REPORT_ERROR(ErrorManagement::ParametersError, "Cannot initialize module NI6723 at address %s", address.Buffer());
 	}
    }
    if(ok)
    {
	int status = module.Configure(0, numOutChannels);
	ok = (status >= 0);
	if(!ok) {
                REPORT_ERROR(ErrorManagement::ParametersError, "Cannot configure module NI6723 at address %s", address.Buffer());
 	}
    }
    if(ok)
    {
	module.Start();
    }
    return ok;
}


uint32 NI6723::GetNumberOfBuffers() const {
    return 1;
}


CLASS_REGISTER(NI6723, "1.0")
}

