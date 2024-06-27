
/*---------------------------------------------------------------------------*/
/*                         Standard header includes                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                         Project header includes                           */
/*---------------------------------------------------------------------------*/
#include "AdvancedErrorManagement.h"
#include "CLASSMETHODREGISTER.h"
#include "StmIn.h"
#include <mdsobjects.h>
#include <stdio.h>

//#define DEBUG 1


/*---------------------------------------------------------------------------*/
/*                           Static definitions                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                           Method definitions                              */
/*---------------------------------------------------------------------------*/
namespace MARTe {
static const int32 FILE_FORMAT_BINARY = 1;
static const int32 FILE_FORMAT_CSV = 2;

StmIn::StmIn() :
        DataSourceI(),
        MessageI() {
	dataSourceMemory = NULL_PTR(char8 *);
	rawChans = NULL_PTR(char8 *);
	port = 0;
}

StmIn::~StmIn() {
//Free allocated buffers
    if (dataSourceMemory != NULL_PTR(char8 *)) {
        GlobalObjectsDatabase::Instance()->GetStandardHeap()->Free(reinterpret_cast<void *&>(dataSourceMemory));
    }
    if (rawChans != NULL_PTR(char8 *)) {
        GlobalObjectsDatabase::Instance()->GetStandardHeap()->Free(reinterpret_cast<void *&>(rawChans));
    }
}

bool StmIn::AllocateMemory() {
    return true;
}

uint32 StmIn::GetNumberOfMemoryBuffers() {
    return 1u;
}

bool StmIn::GetSignalMemoryBuffer(const uint32 signalIdx, const uint32 bufferIdx, void*& signalAddress) {
    bool ok = (dataSourceMemory != NULL_PTR(char8 *));
    if (ok) {
        /*lint -e{613} dataSourceMemory cannot be NULL here*/
        char8 *memPtr = &dataSourceMemory[offsets[signalIdx]];
        signalAddress = reinterpret_cast<void *&>(memPtr);
    }
    return ok;
}

const char8* StmIn::GetBrokerName(StructuredDataI& data, const SignalDirection direction) {
    const char8* brokerName = "";
    if (direction == InputSignals) {
            brokerName = "MemoryMapSynchronisedInputBroker";
    }
    return brokerName;
}

bool StmIn::GetOutputBrokers(ReferenceContainer& inputBrokers, const char8* const functionName, void* const gamMemPtr) {
    return false;
}

bool StmIn::IsSupportedBroker(const SignalDirection direction, const uint32 functionIdx, const uint32 functionSignalIdx, const char8* const brokerClassName)
{
     return true;
}

bool StmIn::GetInputBrokers(ReferenceContainer& inputBrokers, const char8* const functionName, void* const gamMemPtr) {
  
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

bool StmIn::Synchronise() {
  
#ifdef DEBUG
    printf("Synchronise...%d \n", port);
#endif
    uint32 packetLen = 60 + 2 * nOfChannels;
    udpSocket.Read(rawChans, packetLen);
#ifdef DEBUG
    printf("Read...%d \n", packetLen);
#endif
    bool ok = packetLen == 60 + 2 * nOfChannels;
    if(ok)
    {
        memcpy(&dataSourceMemory[offsets[0]], &rawChans[48], 4);
        for(uint32 currChan = 0; currChan < nOfChannels; currChan++)
        {
            uint16 intChan;
            float64 floatChan;
            memcpy(&intChan, &rawChans[60+2*currChan], 2);
            floatChan = 3.3 * intChan/4095.;
            memcpy(&dataSourceMemory[offsets[1]], &floatChan, 8);
        }
    }
  
    counter++;
    
    return ok;
}
 
 
 
/*lint -e{715}  [MISRA C++ Rule 0-1-11], [MISRA C++ Rule 0-1-12]. Justification: NOOP at StateChange, independently of the function parameters.*/
bool StmIn::PrepareNextState(const char8* const currentStateName, const char8* const nextStateName) {
    return true;
}

bool StmIn::Initialise(StructuredDataI& data) {
    bool ok = DataSourceI::Initialise(data);     
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
            REPORT_ERROR(ErrorManagement::Information, "Port shall be specified ");
        }
    }

    if(ok)
    {
	ok = data.MoveRelative("Signals");
    	if(!ok) {
		REPORT_ERROR(ErrorManagement::ParametersError,"Signals node Missing.");
	}
    }
    if(ok)
    {
    	uint32 nOfSignals = data.GetNumberOfChildren();
    	ok = nOfSignals >= 2;
        if(!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError,"Time and at least Chan1 shall be defined.");
        }
        nOfChannels = nOfSignals - 1;
    }
    data.MoveToAncestor(1u);

   return ok;
}


bool StmIn::SetConfiguredDatabase(StructuredDataI& data) {
	
    uint32 totalSignalMemory = 0;
    bool ok = DataSourceI::SetConfiguredDatabase(data);
    if (ok) { // Check that only one GAM is Connected to the MDSReaderNS
        uint32 auxNumberOfFunctions = GetNumberOfFunctions();
        ok = (auxNumberOfFunctions == 1u);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "Exactly one Function allowed to interact with this StmIn DataSource. Number of Functions = %u",
                         auxNumberOfFunctions);
        }
    }
 
    if(ok) //Check Time signal
    {
    	StreamString timeSignalName;
    	ok = GetSignalName(0, timeSignalName);
        if(!ok || timeSignalName != "Time")
        {
            REPORT_ERROR(ErrorManagement::ParametersError, "The first signal shall be Time");
            ok = false;
        }
    }
	if(ok)
	{
	    uint32 nEls;
	    GetSignalNumberOfElements(0, nEls);
	    if(nEls != 1)
	    {
           	REPORT_ERROR(ErrorManagement::ParametersError, "The first signal shall have one element");
	    	ok = false;
 	    }
	    TypeDescriptor firstType = GetSignalType(0);
	    if ((firstType != UnsignedInteger32Bit) && (firstType != SignedInteger32Bit))
 	    {
           	REPORT_ERROR(ErrorManagement::ParametersError, "Time signal shall be either int32 of uint32");
	    	ok = false;
 	    }
	    TypeDescriptor secondType = GetSignalType(1);
	    if (secondType != Float64Bit)
 	    {
           	REPORT_ERROR(ErrorManagement::ParametersError, "Chan1 signal type shall be float64");
	    	ok = false;
 	    }
	}

    if(ok)
    {
        uint32 nBytes;
	    for (uint32 n = 0u; n < 2; n++) {
            offsets[n] = totalSignalMemory;
            ok = GetSignalByteSize(n, nBytes);
            if(ok)
            {
                totalSignalMemory += nBytes;
            }
	    }
    }
    if(ok)
    {
      	dataSourceMemory = reinterpret_cast<char8*>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(totalSignalMemory));
      	rawChans = reinterpret_cast<char8*>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(60+nOfChannels*2));
    }
    counter = 0;
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
    return ok;   
}


uint32 StmIn::GetNumberOfBuffers() const {
    return 1;
}

CLASS_REGISTER(StmIn, "1.0")
}

