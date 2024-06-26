
/*---------------------------------------------------------------------------*/
/*                         Standard header includes                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                         Project header includes                           */
/*---------------------------------------------------------------------------*/
#include "AdvancedErrorManagement.h"
#include "CLASSMETHODREGISTER.h"
#include "SimulinkProxyIn.h"
#include <stdio.h>

//#define DEBUG 1


/*---------------------------------------------------------------------------*/
/*                           Static definitions                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                           Method definitions                              */
/*---------------------------------------------------------------------------*/
namespace MARTe {

SimulinkProxyIn::SimulinkProxyIn() :
        DataSourceI(),
        MessageI() {
	dataSourceMemory = NULL_PTR(char8 *);
    offsets = NULL_PTR(uint32 *);
	totalSignalSize = 0;
    nOfSignals = 0;
	port = 0;
    commSock = NULL_PTR(BasicTCPSocket *);
}

SimulinkProxyIn::~SimulinkProxyIn() {
//Free allocated buffers
    if (dataSourceMemory != NULL_PTR(char8 *)) {
        GlobalObjectsDatabase::Instance()->GetStandardHeap()->Free(reinterpret_cast<void *&>(dataSourceMemory));
    }
    if (offsets != NULL_PTR(uint32 *)) {
        GlobalObjectsDatabase::Instance()->GetStandardHeap()->Free(reinterpret_cast<void *&>(offsets));
    }    
}

bool SimulinkProxyIn::AllocateMemory() {
    return true;
}

uint32 SimulinkProxyIn::GetNumberOfMemoryBuffers() {
    return 1u;
}

bool SimulinkProxyIn::GetSignalMemoryBuffer(const uint32 signalIdx, const uint32 bufferIdx, void*& signalAddress) {
	printf("SIMULINKPROXYIN GETMEM\n");
    bool ok = (dataSourceMemory != NULL_PTR(char8 *));
    if (ok) {
        /*lint -e{613} dataSourceMemory cannot be NULL here*/
        char8 *memPtr = &dataSourceMemory[offsets[signalIdx]];
        signalAddress = reinterpret_cast<void *&>(memPtr);
    }
     return ok;
}

const char8* SimulinkProxyIn::GetBrokerName(StructuredDataI& data, const SignalDirection direction) {
	printf("SIMULINKPROXYIN GETBROKERNAME\n");
    const char8* brokerName = "";
    if (direction == InputSignals) {
            brokerName = "MemoryMapSynchronisedInputBroker";
    }
    return brokerName;
}

bool SimulinkProxyIn::GetOutputBrokers(ReferenceContainer& inputBrokers, const char8* const functionName, void* const gamMemPtr) {
	printf("SIMULINKPROXYIN GETOUTBROKER\n");
    return false;
}

bool SimulinkProxyIn::IsSupportedBroker(const SignalDirection direction, const uint32 functionIdx, const uint32 functionSignalIdx, const char8* const brokerClassName)
{
	printf("SIMULINKPROXYIN ISSUPPORTEDBRO\n");
     return true;
}

bool SimulinkProxyIn::GetInputBrokers(ReferenceContainer& inputBrokers, const char8* const functionName, void* const gamMemPtr) {
  
	printf("SIMULINKPROXYIN GETINPUTBRO\n");
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

bool SimulinkProxyIn::Synchronise() {
    uint32 size = totalSignalSize - 2 * sizeof(int32);
    if(counter == 0 && firstPacketEnabled)
    {
        memset(&dataSourceMemory[offsets[2]], 0, size);
    }
    else
    {
        if(!commSock->Read(&dataSourceMemory[offsets[2]], size))
        {
            REPORT_ERROR(ErrorManagement::FatalError,"Error receiving TCP data");
            commSock->Close();
            commSock = NULL_PTR(BasicTCPSocket *);
            return false;
        }
    }
    *(int32 *)&dataSourceMemory[offsets[0]] = counter;
    counter++;
    uint64 currCounter = HighResolutionTimer::Counter();
    if(startCounter == 0)
    {
        startCounter = currCounter;
    }
    *(uint32 *)&dataSourceMemory[offsets[1]] = (uint32)(HighResolutionTimer::TicksToTime(currCounter, startCounter)* 1E6);
    return true;
}
 
 
 
bool SimulinkProxyIn::PrepareNextState(const char8* const currentStateName, const char8* const nextStateName) {
    return true;
}

bool SimulinkProxyIn::Initialise(StructuredDataI& data) {
    bool ok = DataSourceI::Initialise(data);
    if(ok) {
        ok = data.Read("Port", port);
       if (!ok) {
            REPORT_ERROR(ErrorManagement::Information, "Port shall be specified ");
        }
    }
    if(ok)
    {
        int32 firstPacketEnabledInt = 0;
        ok = data.Read("FirstPacketEnabled", firstPacketEnabledInt);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::Warning, "FirstPacketEnabled not specified, assumed to be 0");
            ok = true;
        }
        firstPacketEnabled = firstPacketEnabledInt != 0;
    }
    if (ok) {
        ok = data.MoveRelative("Signals");
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "Could not move to the Signals section");
        }
    }

    if (ok) {
        nOfSignals = data.GetNumberOfChildren();
        if(nOfSignals < 2)
        {
             REPORT_ERROR(ErrorManagement::ParametersError, "SimulinkProxyIn shall have at least two signals (Counter, Time)");
             ok = false;
        }
    }
    if(ok)
    {
        if(strcmp(data.GetChildName(0), "Counter") || strcmp(data.GetChildName(1), "Time"))
        {
             REPORT_ERROR(ErrorManagement::ParametersError, "The first two signals of SimulinkProxyIn shall shall be Counter and Time");
             ok = false;
        }
    }
    data.MoveToAncestor(1u);
printf("SIMULINKPROXYIN: %d\n", ok);
    return ok;
}


bool SimulinkProxyIn::SetConfiguredDatabase(StructuredDataI& data) {
	printf("SIMULINKPROXYIN CNFIGURE\n");
    bool ok = DataSourceI::SetConfiguredDatabase(data);
    if (ok) { // Check that only one GAM is Connected to the MDSReaderNS
        uint32 auxNumberOfFunctions = GetNumberOfFunctions();
        ok = (auxNumberOfFunctions == 1u);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "Exactly one Function allowed to interact with this SimulinkProxyIn DataSource. Number of Functions = %u",
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
        for (uint32 currSig = 0; currSig < 2; currSig++) //Check type of counter and time
        {
            if(GetSignalType(currSig) != SignedInteger32Bit && GetSignalType(currSig) != UnsignedInteger32Bit)  //Counter, Time
            {
                REPORT_ERROR(ErrorManagement::ParametersError, "Counter and Time shall be of either type int32 or uint32");
                ok = false;
                break;
            }
        }
    }
    if(ok)
    {
   	    offsets  = reinterpret_cast<uint32 *>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(nOfSignals * sizeof(int32)));
    	memset(offsets, 0, nOfSignals * sizeof(int32));
    	totalSignalSize = 0;
    	uint32 nBytes;
	    for (uint32 n = 0u; n < nOfSignals; n++) 
        {
	        offsets[n] = totalSignalSize;
		    GetSignalByteSize(n, nBytes);
	        totalSignalSize += nBytes;
        }
	
    } 
    if(ok)
    {
      	dataSourceMemory = reinterpret_cast<char8*>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(totalSignalSize));
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
            REPORT_ERROR(ErrorManagement::ParametersError, "Cannot bind socket to port %d", port);
        }
    }

    printf("WAITING CONNECTION...\n");
    commSock = serverSock.WaitConnection();
    printf("RICEVUTA CONNESSIONE\n");

    startCounter = 0;
    return ok;   
}


uint32 SimulinkProxyIn::GetNumberOfBuffers() const {
    return 1;
}

CLASS_REGISTER(SimulinkProxyIn, "1.0")
}

