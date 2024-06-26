/**
 * @file NI6255.cpp
 * @brief Source file for class NI6259DIO
 * @date 03/01/2017
 * @author Andre Neto
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
 * the class NI6255 (public, protected, and private). Be aware that some
 * methods, such as those inline could be defined on the header file, instead.
 */

/*---------------------------------------------------------------------------*/
/*                         Standard header includes                          */
/*---------------------------------------------------------------------------*/
#include <fcntl.h>

/*---------------------------------------------------------------------------*/
/*                         Project header includes                           */
/*---------------------------------------------------------------------------*/
#include "AdvancedErrorManagement.h"
#include "MemoryMapSynchronisedInputBroker.h"
#include "MemoryMapSynchronisedOutputBroker.h"
#include "NI6255.h"

/*---------------------------------------------------------------------------*/
/*                           Static definitions                              */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/*                           Method definitions                              */
/*---------------------------------------------------------------------------*/
namespace MARTe {
NI6255::NI6255() : DataSourceI() {
    numInputChannels = 0;
}



NI6255::~NI6255() {
}
    
bool NI6255::AllocateMemory() {
    return true;
}

uint32 NI6255::GetNumberOfMemoryBuffers() {
    return 1u;
}
bool NI6255::GetSignalMemoryBuffer(const uint32 signalIdx, const uint32 bufferIdx, void*& signalAddress) {
    bool ok;
    ok = (signalIdx < numInputChannels);
    if (ok) {
	signalAddress = static_cast<void *> (&inputChannels[signalIdx]);
    }
    return ok;
}
bool NI6255::IsSupportedBroker(const SignalDirection direction, const uint32 functionIdx, const uint32 functionSignalIdx, const char8* const brokerClassName)
{
     return true;
}

const char8* NI6255::GetBrokerName(StructuredDataI& data, const SignalDirection direction) {
    const char8 *brokerName = NULL_PTR(const char8 *);
    if (direction == InputSignals) 
    {
        brokerName = "MemoryMapSynchronisedInputBroker";
    }
    return brokerName;
}

bool NI6255::GetInputBrokers(ReferenceContainer& inputBrokers, const char8* const functionName, void* const gamMemPtr) {
  REPORT_ERROR(ErrorManagement::Debug, "GetInputBrokers");
    ReferenceT<MemoryMapSynchronisedInputBroker> broker("MemoryMapSynchronisedInputBroker");
    bool ok = broker.IsValid();

    if (ok) {
        ok = broker->Init(InputSignals, *this, functionName, gamMemPtr);
    }
    if (ok) {
        ok = inputBrokers.Insert(broker);
    }
 REPORT_ERROR(ErrorManagement::Debug, "GetInputBrokers fatta %d", ok);
 
    return ok;
}

bool NI6255::GetOutputBrokers(ReferenceContainer& outputBrokers, const char8* const functionName, void* const gamMemPtr) {
 REPORT_ERROR(ErrorManagement::Debug, "GetOutputBrokers");   
	return false;
}

bool NI6255::PrepareNextState(const char8* const currentStateName, const char8* const nextStateName) {
    return true;
}

bool NI6255::Initialise(StructuredDataI& data) {
 REPORT_ERROR(ErrorManagement::Debug, "INITIALISE");   
    bool ok = DataSourceI::Initialise(data);
    if (ok) {
      ok = data.Read("Address", deviceAddress);
      if(!ok)
      {
	  REPORT_ERROR(ErrorManagement::ParametersError,"NI6255 device Address shall be specified");
	  return ok;
      }
    }
    return ok;
}

bool NI6255::SetConfiguredDatabase(StructuredDataI& data) {
    REPORT_ERROR(ErrorManagement::Debug, "PARTE SetConfiguredDatabase");   
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
    numInputChannels = GetNumberOfSignals();
    if(numInputChannels < 1 || numInputChannels > 80)
    {
	REPORT_ERROR(ErrorManagement::ParametersError, "The declared number %d of signals must be between 1 and 80", numInputChannels);
	return false;
    }
    for(uint32 sigIdx = 0; sigIdx < numInputChannels; sigIdx++)
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
    int status = module.Init((char *)deviceAddress.Buffer());
    if(status < 0)
    {
	REPORT_ERROR(ErrorManagement::FatalError,"Cannot initialize device NI6255 at address %s", deviceAddress.Buffer());
	return false;;
    }
    status = module.InitDMA();
    if(status < 0)
    {
	REPORT_ERROR(ErrorManagement::FatalError,"Cannot initialize DMA for NI6255 device at address %s", deviceAddress.Buffer());
	return false;;
    }
    module.Reset();
    status = module.Configure(numInputChannels,0);
    if(status < 0)
    {
	REPORT_ERROR(ErrorManagement::FatalError,"Cannot configure NI6255 device at address %s", deviceAddress.Buffer());
	return false;;
    }
    status = module.ConfigureDMA();
    if(status < 0)
    {
	REPORT_ERROR(ErrorManagement::FatalError,"Cannot configure DMA for NI6255device at address %s", deviceAddress.Buffer());
	return false;;
    }
    status = module.StartDMA();
    if(status < 0)
    {
	REPORT_ERROR(ErrorManagement::FatalError,"Cannot start DMA for NI6255device at address %s", deviceAddress.Buffer());
	return false;;
    }
    status = module.Start();
    if(status < 0)
    {
	REPORT_ERROR(ErrorManagement::FatalError,"Cannot start NI6255device at address %s", deviceAddress.Buffer());
	return false;;
    }
    return ok;
}

bool NI6255:: Synchronise()
{
//The first instance at the first Synchronise will open the channe descriptor
    REPORT_ERROR(ErrorManagement::Debug, "PARTE SYNCHRONIZE");
    module.PollDMA();
    module.ReadDMA((u32 *)inputChannelBuffers, numInputChannels);
    for(size_t i = 0; i < numInputChannels; i++)
      inputChannels[i] = inputChannelBuffers[i];
    return true;
}


CLASS_REGISTER(NI6255, "1.0")

}

