
/*---------------------------------------------------------------------------*/
/*                         Standard header includes                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                         Project header includes                           */
/*---------------------------------------------------------------------------*/
#include "AdvancedErrorManagement.h"
#include "CLASSMETHODREGISTER.h"
#include "TcpIn.h"
#include <stdio.h>

//#define DEBUG 1


/*---------------------------------------------------------------------------*/
/*                           Static definitions                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                           Method definitions                              */
/*---------------------------------------------------------------------------*/
namespace MARTe {

TcpIn::TcpIn() :
        DataSourceI(),
        MessageI() {
	dataSourceMemory = NULL_PTR(char8 *);
	offsets = NULL_PTR(uint32 *);
	nOfSignals = 0;
	port = 0;
    commSock = NULL_PTR(BasicTCPSocket *);
}

TcpIn::~TcpIn() {
//Free allocated buffers
    if (dataSourceMemory != NULL_PTR(char8 *)) {
        GlobalObjectsDatabase::Instance()->GetStandardHeap()->Free(reinterpret_cast<void *&>(dataSourceMemory));
    }
    if (offsets != NULL_PTR(uint32 *)) {
        GlobalObjectsDatabase::Instance()->GetStandardHeap()->Free(reinterpret_cast<void *&>(offsets));
    }    
}

bool TcpIn::AllocateMemory() {
    return true;
}

uint32 TcpIn::GetNumberOfMemoryBuffers() {
    return 1u;
}

bool TcpIn::GetSignalMemoryBuffer(const uint32 signalIdx, const uint32 bufferIdx, void*& signalAddress) {
    bool ok = (dataSourceMemory != NULL_PTR(char8 *));
    if (ok) {
        /*lint -e{613} dataSourceMemory cannot be NULL here*/
        char8 *memPtr = &dataSourceMemory[offsets[signalIdx]];
        signalAddress = reinterpret_cast<void *&>(memPtr);
    }
     return ok;
}

const char8* TcpIn::GetBrokerName(StructuredDataI& data, const SignalDirection direction) {
    const char8* brokerName = "";
    if (direction == InputSignals) {
            brokerName = "MemoryMapSynchronisedInputBroker";
    }
    return brokerName;
}

bool TcpIn::GetOutputBrokers(ReferenceContainer& inputBrokers, const char8* const functionName, void* const gamMemPtr) {
    return false;
}

bool TcpIn::IsSupportedBroker(const SignalDirection direction, const uint32 functionIdx, const uint32 functionSignalIdx, const char8* const brokerClassName)
{
     return true;
}

bool TcpIn::GetInputBrokers(ReferenceContainer& inputBrokers, const char8* const functionName, void* const gamMemPtr) {
  
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

bool TcpIn::Synchronise() {
    for(uint32 i = 0; i < nOfTcpSignals; i++)
    {
        uint32 size = sizeof(float64);
        if(!commSock->Read(dataSourceMemory + offsets[i], size))
        {
            REPORT_ERROR(ErrorManagement::FatalError,"Error receiving TCP data");
            commSock->Close();
            commSock = NULL_PTR(BasicTCPSocket *);
           return false;
        }
    }
    for(uint32_t i = nOfTcpSignals; i < nOfSignals; i++)
    {
        double zero = 0;
        memcpy(dataSourceMemory + offsets[i], &zero, sizeof(float64));
    }    
  return true;
}
 
 
 
bool TcpIn::PrepareNextState(const char8* const currentStateName, const char8* const nextStateName) {
    return true;
}

bool TcpIn::Initialise(StructuredDataI& data) {
    bool ok = DataSourceI::Initialise(data);
    if(ok) {
        ok = data.Read("Port", port);
       if (!ok) {
            REPORT_ERROR(ErrorManagement::Information, "Port shall be specified ");
        }
    }
    if(ok) {
        ok = data.Read("NumTcpSignals", nOfTcpSignals);
       if (!ok) {
            REPORT_ERROR(ErrorManagement::Information, "Number of TCP signals shall be specified ");
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
    	nOfSignals = data.GetNumberOfChildren();
    	ok = nOfSignals >= nOfTcpSignals;
	if(!ok) {
		REPORT_ERROR(ErrorManagement::ParametersError,"ANumber of signals canno be less than number of TCP signals");
	}
    }
    data.MoveToAncestor(1u);

   return ok;
}


bool TcpIn::SetConfiguredDatabase(StructuredDataI& data) {
	
    uint32 totalSignalMemory = 0;
    bool ok = DataSourceI::SetConfiguredDatabase(data);
    if (ok) { // Check that only one GAM is Connected to the MDSReaderNS
        uint32 auxNumberOfFunctions = GetNumberOfFunctions();
        ok = (auxNumberOfFunctions == 1u);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "Exactly one Function allowed to interact with this TcpIn DataSource. Number of Functions = %u",
                         auxNumberOfFunctions);
        }
    }
    if (ok) { //read number of nodes per function numberOfNodeNames
        //0u (second argument) because previously it is checked
        ok = GetFunctionNumberOfSignals(InputSignals, (uint32)0u, nOfSignals);        //0u (second argument) because previously it is checked
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "GetFunctionNumberOfSignals() returned false");
        }
    }


	if(ok)
	{
        for(uint32 i = 0; i < nOfSignals; i++)
        {
	        uint32 nEls;
	        GetSignalNumberOfElements(i, nEls);
	        if(nEls != 1)
	        {
           	    REPORT_ERROR(ErrorManagement::ParametersError, "Signals shall have one element");
	    	    ok = false;
 	        }
	        TypeDescriptor currType = GetSignalType(i);
	        if ((currType != Float64Bit))
 	        {
           	    REPORT_ERROR(ErrorManagement::ParametersError, "Signals shall be Float64");
	    	    ok = false;
 	        }
        }
	}
    if(ok)
    {
   	    offsets  = reinterpret_cast<uint32 *>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(nOfSignals * sizeof(int32)));
    	memset(offsets, 0, nOfSignals * sizeof(int32));
    	totalSignalMemory = 0;
    	uint32 nBytes;
	    for (uint32 n = 0u; n < nOfSignals; n++) 
        {
	        offsets[n] = totalSignalMemory;
		    GetSignalByteSize(n, nBytes);
	        totalSignalMemory += nBytes;
        }
	
    } 
    if(ok)
    {
      	dataSourceMemory = reinterpret_cast<char8*>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(totalSignalMemory));
    }
    if (ok) {
        for (uint32 n = 0u; (n < nOfSignals) && ok; n++) {
            uint32 nSamples;
            ok = GetFunctionSignalSamples(InputSignals, 0u, n, nSamples);
            if (ok) {
                ok = (nSamples == 1u);
            }
            if (!ok) {
                REPORT_ERROR(ErrorManagement::ParametersError, "The number of samples shall be exactly 1");
            }
        }
    }
    if(ok)
    {
   	    ok = serverSock.Open();
        if(!ok)
        {
            REPORT_ERROR(ErrorManagement::ParametersError, "Cannot open socket");
        }
    }
    if(ok)
    {
   	    ok = serverSock.Listen(port);
        if(!ok)
        {
            REPORT_ERROR(ErrorManagement::ParametersError, "Cannot bind socket ");
        }
    }

    printf("WAITING CONNECTION...\n");
    commSock = serverSock.WaitConnection();
    printf("RICEVUTA CONNESSIONE\n");
    return ok;   
}


uint32 TcpIn::GetNumberOfBuffers() const {
    return 1;
}

CLASS_REGISTER(TcpIn, "1.0")
}

