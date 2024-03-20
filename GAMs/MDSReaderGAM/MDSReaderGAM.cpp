#define DLL_API

/*---------------------------------------------------------------------------*/
/*                         Standard header includes                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                         Project header includes                           */
/*---------------------------------------------------------------------------*/

#include "AdvancedErrorManagement.h"
#include "MDSReaderGAM.h"

#define DEBUG

/*---------------------------------------------------------------------------*/
/*                           Static definitions                              */
/*---------------------------------------------------------------------------*/

/*lint -estring(1960, "*MDSplus::*") -estring(1960, "*std::*") Ignore errors that do not belong to this DataSource namespace*/

/*---------------------------------------------------------------------------*/
/*                           Method definitions                              */
/*---------------------------------------------------------------------------*/

namespace MARTe {

MDSReaderGAM::MDSReaderGAM() : GAM() {
    tree = NULL_PTR(MDSplus::Tree *);
    dataExpr = NULL_PTR(StreamString *);
    timebaseExpr = NULL_PTR(StreamString *);
    numberOfNodeNames = 0u;
    shotNumber = 0;
    numberOfElements = NULL_PTR(uint32 *);
    outSignalsMemory = NULL_PTR(void **);
    currentTime = 0;
    endNode = NULL_PTR(bool *);
    outTypes = NULL_PTR(uint32 *);
    signalData = NULL_PTR(float64 **);
    signalTimebase = NULL_PTR(float64 **);
    numSignalSamples = NULL_PTR(uint32 *);
    lastSignalSample = NULL_PTR(uint32 *);
    nElements = NULL_PTR(uint32 *);
    useColumnOrder = NULL_PTR(bool *);
}

/*lint -e{1551} the destructor must guarantee that the MDSplus are deleted and the shared memory freed*/
MDSReaderGAM::~MDSReaderGAM() {

    if (tree != NULL_PTR(MDSplus::Tree *)) {
        delete tree;
        tree = NULL_PTR(MDSplus::Tree *);
    }

    if (dataExpr != NULL_PTR(StreamString *)) {
        delete[] dataExpr;
        dataExpr = NULL_PTR(StreamString *);
    }

    if (timebaseExpr != NULL_PTR(StreamString *)) {
        delete[] timebaseExpr;
        timebaseExpr = NULL_PTR(StreamString *);
    }

    if (outSignalsMemory != NULL_PTR(void **)) {
        delete[] outSignalsMemory;
        outSignalsMemory = NULL_PTR(void **);
    }
    if (endNode != NULL_PTR(bool *)) {
        delete[] endNode;
        endNode = NULL_PTR(bool *);
    }
    if (signalData != NULL_PTR(float64 **)) {
        delete[] signalData;
        signalData = NULL_PTR(float64 **);
    }
    if (signalTimebase != NULL_PTR(float64 **)) {
        delete[] signalTimebase;
        signalTimebase = NULL_PTR(float64 **);
    }
    if (numSignalSamples != NULL_PTR(uint32 *)) {
        delete[] numSignalSamples;
        numSignalSamples = NULL_PTR(uint32 *);
    }
    if (lastSignalSample != NULL_PTR(uint32 *)) {
        delete[] lastSignalSample;
        lastSignalSample = NULL_PTR(uint32 *);
    }
    if (useColumnOrder != NULL_PTR(bool *)) {
        delete[] useColumnOrder;
        useColumnOrder = NULL_PTR(bool *);
    }
    if (nElements != NULL_PTR(uint32 *)) {
        delete[] nElements;
        nElements = NULL_PTR(uint32 *);
    }
    if (outTypes != NULL_PTR(uint32 *)) {
        delete[] outTypes;
        outTypes = NULL_PTR(uint32 *);
    }
    
}

bool MDSReaderGAM::Initialise(StructuredDataI& data) {
    bool ok = GAM::Initialise(data);
#ifdef DEBUG    
    std::cout << "MDS READER GAM INIT" <<std::endl;
#endif
    if (ok) {
        ok = data.Read("TreeName", treeName);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "TreeName shall be specified");
        }
    }
    if (ok) {
        ok = data.Read("ShotNumber", shotNumber);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::Information, "ShotNumber shall be specified");
        }
        else {
            ok = (shotNumber >= 0);
            if (!ok) {
                REPORT_ERROR(ErrorManagement::ParametersError, "ShotNumber shall be 0 (last available shot) or positive");
            }
        }
    }
    if (ok) {
        ok = OpenTree();
    }
    if (ok) {
        ok = data.MoveRelative("InputSignals");
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "Could not move to the InputSignals section");
        }
        else
        {
            uint32 numInputSignals = data.GetNumberOfChildren();
            ok = (numInputSignals == 1);
            if(!ok) {
                REPORT_ERROR(ErrorManagement::ParametersError, "The number of input signal shall be one (the time)");
            }
        }
	    // Back to the GAM node
	    data.MoveToAncestor(1);
    }
    if (ok) {
        ok = data.MoveRelative("OutputSignals");
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "Could not move to the InputSignals section");
        }
        else
        {
            numberOfNodeNames = data.GetNumberOfChildren();
            ok = (numberOfNodeNames > 0);
            if(!ok) {
                REPORT_ERROR(ErrorManagement::ParametersError, "There shall be at least one output signal");
            }
            else
            {
	            dataExpr = new StreamString[numberOfNodeNames];
                timebaseExpr = new StreamString[numberOfNodeNames];
	            useColumnOrder = new bool[numberOfNodeNames];
                for (uint32 sigIdx = 0; sigIdx < numberOfNodeNames; sigIdx++) 
                {
		// Move the cursor to output signal
		            ok = data.MoveToChild(sigIdx);
		            if(!ok) {
			            REPORT_ERROR(ErrorManagement::ParametersError,
							"OutputSignals node has no child.");
                        break;
                    }
		// Expression is read and stored in expressionStringArray
		            ok = data.Read("DataExpr", dataExpr[sigIdx]);
		            if(!ok) {
			            REPORT_ERROR(ErrorManagement::ParametersError,
							"DataExpr leaf is missing (or is not a string) for output signal %s.",
							data.GetName());
                            break;
                    }
		            ok = data.Read("TimebaseExpr", timebaseExpr[sigIdx]);
		            if(!ok) {
			            REPORT_ERROR(ErrorManagement::ParametersError,
							"TimebaseExpr leaf is missing (or is not a string) for output signal %s.",
							data.GetName());
                            break;
                    }
                    uint32 useColumnOrderId;
		            ok = data.Read("UseColumnOrder", useColumnOrderId);
		            if (ok)
		            {
		                ok = (useColumnOrderId == 0 || useColumnOrderId == 1);
		                if (!ok) {
			                REPORT_ERROR(ErrorManagement::ParametersError, "UseColumnOrder shall be 0 or 1");
                            break;
		                }
		                useColumnOrder[sigIdx] = (useColumnOrderId == 1);
		            }
		            else
                    {
                        useColumnOrder[sigIdx] = false;
		                ok = true; //Parameter is optional
	                }
                    ok = data.MoveToAncestor(1u);
                    if (!ok) { //Should never happen
                        REPORT_ERROR(ErrorManagement::ParametersError, "Cannot move to the the immediate ancestor");
                        break;
                    }
                }
            }

		}
		if(ok)
        {
		// Pointer must be reset to the GAM node
		    ok = data.MoveToAncestor(1);
		    if(!ok){
			    REPORT_ERROR(ErrorManagement::InitialisationError,
						"Failed MoveToAncestor() from %s", data.GetName());
		    }        
        }
    }

    if (ok) 
    { //read DataManagement from originalSignalInformation
        dataManagement = new uint8[numberOfNodeNames];
        nodeSamplingTime = new float64[numberOfNodeNames];
	    signalData = new float64 *[numberOfNodeNames];
	    signalTimebase = new float64 *[numberOfNodeNames];
	    numSignalSamples = new uint32[numberOfNodeNames];
	    lastSignalSample = new uint32[numberOfNodeNames];
	    nElements = new uint32[numberOfNodeNames];
        ok = data.MoveRelative("OutputSignals");
        if (!ok) 
        {
            REPORT_ERROR(ErrorManagement::ParametersError, "Cannot move to the OutputSignals");
        }
        //lint -e{613} Possible use of null pointer. The pointer usage is protected by the ok variable.
        for (uint32 i = 0u; (i < numberOfNodeNames) && ok; i++) {
            ok = data.MoveToChild(i);
            //ok = data.MoveRelative(data.GetChildName(i));
            if (!ok) {
                uint32 auxIdx = i;
                REPORT_ERROR(ErrorManagement::ParametersError, "Cannot move to child %u", auxIdx);
            }
            if (ok) {
                ok = data.Read("DataManagement", dataManagement[i]);
                if (!ok) {
                    dataManagement[i] = 1; //Interpolation is the default
                    ok = true;
                }
            }
            if (ok) {
                ok = (dataManagement[i] == 1 || dataManagement[i] == 2);
                if (!ok) {
                    uint32 auxIdx = i;
                    REPORT_ERROR(
                            ErrorManagement::ParametersError,
                            "Invalid DataManagement value. It could be1 (linear interpolation) or 2 (hold last value). dataManagement[%d] = %d",
                            auxIdx, dataManagement[auxIdx]);
                }
            }
            if (ok) {
	            ok = GetNodeDataAndSamplingTime(i, signalData[i], nElements[i], signalTimebase[i],  numSignalSamples[i], nodeSamplingTime[i]);
	        }

#ifdef DEBUG
	        std::cout << "Number of elements: " << nElements[i] << "   Number of signal samples: " << numSignalSamples[i] << std::endl;
	        for(uint32 j  =0; j <  numSignalSamples[i]; j++) {
		        std::cout << signalData[i][j] << "  "; 
	        }
	        std::cout<< std::endl;	    
#endif	    
            if (ok) 
            { //check time of doing nothing option
        	    lastSignalSample[i] = 0;
                ok = data.MoveToAncestor(1u);
                if (!ok) { //Should never happen
                    REPORT_ERROR(ErrorManagement::ParametersError, "Cannot move to the the immediate ancestor");
                }
            }
        }
    }
    if(ok)
    {
        ok = data.MoveToAncestor(1u);
        if (!ok) { //Should never happen
            REPORT_ERROR(ErrorManagement::ParametersError, "Cannot move to the the immediate ancestor");
        }
    }

    if (ok) {
    //Check if there are any Message elements set
        nOfMessages = 0;
        if (Size() > 0u) {
            ReferenceT<ReferenceContainer> msgContainer = Get(0u);
            if (msgContainer.IsValid()) {
                uint32 j;
                nOfMessages = msgContainer->Size();
                signalsEndedMsg = new MARTe::ReferenceT<MARTe::Message>[nOfMessages];
                for (j = 0u; (j < nOfMessages) && (ok); j++) {
                    ReferenceT<Message> msg = msgContainer->Get(j);
                    ok = msg.IsValid();
                    if (ok) {
                        StreamString msgName = msg->GetName();
                    //if (msgName == "SignalsEnded") {
                        signalsEndedMsg[j] = msg;
                    //}
                    //else {
                    //     REPORT_ERROR(ErrorManagement::ParametersError, "Message %s is not supported.", msgName.Buffer());
                    //      ok = false;
                    //   }
                    }
                    else {
                        REPORT_ERROR(ErrorManagement::ParametersError, "Found an invalid Message in container %s", msgContainer->GetName());
                        ok = false;
                    }

                }
            }
        }
    }
#ifdef DEBUG
        std::cout << "END MDS READER GAM INIT Ok: " << ok << std::endl;
#endif
    return ok;
}

bool MDSReaderGAM::Setup() {
    bool ok = true;
    std::cout << "SETUP  MDS READER" << std::endl<< std::endl;

    inputType =  GetSignalType(InputSignals, 0);
    ok = ((inputType == UnsignedInteger64Bit)||(inputType == SignedInteger64Bit)||(inputType == UnsignedInteger32Bit)||(inputType == SignedInteger64Bit));
    if(!ok)
    {
         REPORT_ERROR(ErrorManagement::ParametersError, "Input signal type shall be either int64, uint64, int32, uint32");
    }
    if(ok)
    {
        for (uint32 n = 0u; (n < numberOfNodeNames) && ok; n++) {
            uint32 nSamples = 0u;
            ok = GetSignalNumberOfSamples(OutputSignals, n, nSamples);
            if (ok) {
                ok = (nSamples == 1u);
            }
            if (!ok) {
                REPORT_ERROR(ErrorManagement::ParametersError, "The number of samples shall be exactly 1", nSamples);
            }
        }
    }    

    if (ok) { 
        inType = convertType(GetSignalType(InputSignals, 0));
        outTypes = new uint32[numberOfNodeNames];
        for (uint32 i = 0u; (i < numberOfNodeNames) && ok; i++) {
            uint32 currNElements;
            ok = GetSignalNumberOfElements(OutputSignals, i, currNElements);
            if (!ok) {
                REPORT_ERROR(ErrorManagement::ParametersError, "Cannot read NumberOfElements");
                break;
            }
            ok = (nElements[i] == currNElements);
            if (!ok) {
                REPORT_ERROR(ErrorManagement::ParametersError, "Inconsistent NumberOfElements: expected %d found %d",
                    nElements[i], currNElements);
                break;
            }
            outTypes[i] = convertType(GetSignalType(OutputSignals, i));
            if(outTypes[i] == 0)
            {
                ok = false;
                REPORT_ERROR(ErrorManagement::ParametersError, "Invalid type for out signal %d", i);
                break;
           }
        }
    }
    if (ok) { //Count and allocate memory for dataSourceMemory, lastValue and lastTime
        inSignalMemory = GetInputSignalMemory(0);
        outSignalsMemory = new void*[numberOfNodeNames];
        for (uint32 i = 0u; (i < numberOfNodeNames) && ok; i++) 
        {
             outSignalsMemory[i] = GetOutputSignalMemory(i);
        }
    }
 
    if (ok) {
        endNode = new bool[numberOfNodeNames];
        for (uint32 i = 0u; i < numberOfNodeNames; i++) {
            endNode[i] = false;
        }
    }

    numCycles = 0;
    return ok;
}



bool MDSReaderGAM::Execute() {
    switch(inType) {
        case MDSREADERGAM_INT8:
             currentTime = * reinterpret_cast<int8 *>(inSignalMemory);
            break;
        case MDSREADERGAM_UINT8:
             currentTime = * reinterpret_cast<uint8 *>(inSignalMemory);
            break;
       case MDSREADERGAM_INT16:
             currentTime = * reinterpret_cast<int16 *>(inSignalMemory);
            break;
        case MDSREADERGAM_UINT16:
             currentTime = * reinterpret_cast<uint16 *>(inSignalMemory);
            break;
       case MDSREADERGAM_INT32:
             currentTime = * reinterpret_cast<int32 *>(inSignalMemory);
            break;
        case MDSREADERGAM_UINT32:
             currentTime = * reinterpret_cast<uint32 *>(inSignalMemory);
            break;
       case MDSREADERGAM_INT64:
             currentTime = * reinterpret_cast<int64 *>(inSignalMemory);
            break;
        case MDSREADERGAM_UINT64:
             currentTime = * reinterpret_cast<uint64 *>(inSignalMemory);
            break;
    }
    currentTime /= 1E6;
#ifdef DEBUG
    std::cout << "MDSReaderGAM - Current time: " << currentTime << std::endl; 
#endif   
    for (uint32 i = 0u; i < numberOfNodeNames; i++) {
        endNode[i] = !GetDataNode(i);
    }
    if(AllNodesEnd() && !signalsEndedNotified)
    {
	    signalsEndedNotified = true;
//std::cout << "FINITO TUTTI" << std::endl;
	    notifySignalsEnded();
    }
    else
    {
	    if(!AllNodesEnd())
	    {
	        signalsEndedNotified = false;
	    }
    }
    numCycles++;
    return true;
}

bool MDSReaderGAM::AllNodesEnd() const {
    bool ret = true;
    if (endNode != NULL_PTR(bool *)) {
        for (uint32 i = 0u; (i < numberOfNodeNames) && ret; i++) {
            ret = endNode[i];
        }
    }
    return ret;
}

bool MDSReaderGAM::OpenTree() {
    bool ret = true;
    try {
        tree = new MDSplus::Tree(treeName.Buffer(), shotNumber);
    }
    catch (const MDSplus::MdsException &exc) {
        REPORT_ERROR_STATIC(ErrorManagement::ParametersError, "Fail opening tree %s with shotNumber %d:  %s", treeName.Buffer(), shotNumber, exc.what());
        ret = false;
    }
    return ret;
}





/**
 * -1--> end data (not found)
 * 0--> is not end of data but not found. Case signal trigger (the segment are not continuous)
 * 1--> time found in a segment
 */

bool MDSReaderGAM::GetDataNode(const uint32 nodeNumber) {
  
//  std::cout << "GET DATA NODE " << nodeNumber << "  nElements: " << nElements[nodeNumber] << "Management: " << (int)dataManagement[nodeNumber] <<std::endl;  
  
    switch(dataManagement[nodeNumber])  {
	    case 1: //Interpolation
	    {
	        uint32 startIdx;
//std::cout << "signal timebase 0: " << signalTimebase[nodeNumber][0] << " num signal samples" << numSignalSamples[nodeNumber] << "  current time: " << currentTime << std::endl;
	        if(signalTimebase[nodeNumber][0] > currentTime)
	        {
		        for(uint32 i = 0; i < nElements[nodeNumber]; i++)
		        {
		            CopyValue(nodeNumber, i, 0.);
		        }
		        return true;
	        }
	        if(signalTimebase[nodeNumber][numSignalSamples[nodeNumber]-1] < currentTime)
	        {
		        for(uint32 i = 0; i < nElements[nodeNumber]; i++)
		        {
		            CopyValue(nodeNumber, i, 0.);
		        }
		        return false;
	        }
	        for(startIdx = lastSignalSample[nodeNumber]; startIdx < numSignalSamples[nodeNumber]-1 && signalTimebase[nodeNumber][startIdx+1] < currentTime; startIdx++);
	        for(uint32 i = 0; i < nElements[nodeNumber]; i++)
	        {
		        float64 interpValue = signalData[nodeNumber][startIdx*nElements[nodeNumber]+i] + (currentTime - signalTimebase[nodeNumber][startIdx])*
		            (signalData[nodeNumber][(startIdx+1)*nElements[nodeNumber]+i] - signalData[nodeNumber][startIdx*nElements[nodeNumber]+i])/(signalTimebase[nodeNumber][startIdx+1] - signalTimebase[nodeNumber][startIdx]);
		        CopyValue(nodeNumber, i, interpValue);
// std::cout << "startIdx: " << startIdx << "   Current time: " << currentTime << "Current Index: " << startIdx*nElements[nodeNumber]+i << 
// "Val1: "<< signalData[nodeNumber][startIdx*nElements[nodeNumber]+i] <<
// "Interp Val: " << interpValue <<std::endl;
	        }
	        lastSignalSample[nodeNumber] = startIdx;
	        return true;
	    }
	    case 2: //Closest Sample
	    {
	        uint32 startIdx;
	        if(lastSignalSample[nodeNumber] == 0 && signalTimebase[nodeNumber][lastSignalSample[nodeNumber]] > currentTime)
	        {
		        for(uint32 i = 0; i < nElements[nodeNumber]; i++)
		        {
		            CopyValue(nodeNumber, i, 0.);
		        }
		        return true;
	        }
	        if(lastSignalSample[nodeNumber] >= numSignalSamples[nodeNumber] - 1 && signalTimebase[nodeNumber][lastSignalSample[nodeNumber]] < currentTime)
	        {
		        for(uint32 i = 0; i < nElements[nodeNumber]; i++)
		        {
		            CopyValue(nodeNumber, i, 0.);
		        }
		        return false;
	        }
	        for(startIdx = lastSignalSample[nodeNumber]; startIdx < numSignalSamples[nodeNumber]-1 && signalTimebase[nodeNumber][startIdx+1] <= currentTime; startIdx++);
	        if((currentTime - signalTimebase[nodeNumber][startIdx]) < (signalTimebase[nodeNumber][startIdx+1] - currentTime))
	        {
		        for(uint32 i = 0; i < nElements[nodeNumber]; i++)
		        {
		            CopyValue(nodeNumber, i, signalData[nodeNumber][startIdx*nElements[nodeNumber]+i]);
		        }
	        }
	        else
	        {
		        if(startIdx < numSignalSamples[nodeNumber]-1)
		        {
		            for(uint32 i = 0; i < nElements[nodeNumber]; i++)
		            {
			            CopyValue(nodeNumber, i, signalData[nodeNumber][(startIdx+1)*nElements[nodeNumber]+i]);
		            }
		        }
		        else
		        {
		            for(uint32 i = 0; i < nElements[nodeNumber]; i++)
		            {
		                CopyValue(nodeNumber, i, signalData[nodeNumber][startIdx*nElements[nodeNumber]+i]);
		            }
		        }
	        }
	        lastSignalSample[nodeNumber] = startIdx;
	        return true;
	    }
	    default: return false; //Never happens
    }
}
	
    
 
//lint -e{613} Possible use of null pointer. Not possible. If initialisation fails this function is not called.
void MDSReaderGAM::CopyValue(const uint32 idxNumber, uint32 element, float64 value) 
{
    switch(outTypes[idxNumber])  {
        case MDSREADERGAM_UINT8:
            CopyValueTemplate<uint8>(idxNumber, element, value);
            break;
        case MDSREADERGAM_INT8:
            CopyValueTemplate<int8>(idxNumber, element, value);
            break;
        case MDSREADERGAM_UINT16:
            CopyValueTemplate<uint16>(idxNumber, element, value);
            break;
        case MDSREADERGAM_INT16:
            CopyValueTemplate<int16>(idxNumber, element, value);
            break;
       case MDSREADERGAM_UINT32:
            CopyValueTemplate<uint32>(idxNumber, element, value);
            break;
       case MDSREADERGAM_INT32:
            CopyValueTemplate<int32>(idxNumber, element, value);
            break;
       case MDSREADERGAM_UINT64:
            CopyValueTemplate<uint64>(idxNumber, element, value);
            break;
       case MDSREADERGAM_INT64:
            CopyValueTemplate<int64>(idxNumber, element, value);
            break;
       case MDSREADERGAM_FLOAT32:
            CopyValueTemplate<float32>(idxNumber, element, value);
            break;
       case MDSREADERGAM_FLOAT64:
            CopyValueTemplate<float64>(idxNumber, element, value);
    }
}


bool MDSReaderGAM::GetNodeDataAndSamplingTime(const uint32 idx, float64 * &data, uint32 &numElements, float64 * &timebase, uint32 &numDataSamples,
            float64 &tDiff) const
{
    int nDims;
    try {
        REPORT_ERROR(ErrorManagement::Information, "Expr: %s", dataExpr[idx].Buffer());        
        MDSplus::Data *nodeData = tree->tdiCompile(dataExpr[idx].Buffer());
	    int dataSamples;
	    MDSplus::Data *evalData = tree->tdiData(nodeData);
        REPORT_ERROR(ErrorManagement::Information, "EvalData: %s", evalData->decompile());
	    MDSplus::deleteData(nodeData);
	    int *shape = evalData->getShape(&nDims);
	    if(useColumnOrder[idx])
	    {
	        numDataSamples = shape[nDims-1];
	        numElements = 1;
	        for(int i = 0; i < nDims-1; i++)
		        numElements *= shape[i];
	        float64 *currData = evalData->getDoubleArray(&dataSamples);
	        data = new float64[dataSamples];
	        for(uint32 i = 0; i < numDataSamples; i++)
	        {
		        for(uint32 j = 0; j < numElements; j++)
		        {
		            data[i*numElements + j] = currData[j*numDataSamples+i];
		        }
	         }
	        delete [] currData;
	    }
	    else
	    {
	        numDataSamples = shape[0];
	        numElements = 1;
	        for(int i = 1; i < nDims; i++)
		        numElements *= shape[i];
	        data = evalData->getDoubleArray(&dataSamples);
	    }
        MDSplus::deleteData(evalData);
    }catch(MDSplus::MdsException &exc)
    {
	    REPORT_ERROR(ErrorManagement::ParametersError, "Cannot read data for signal %s: %s", dataExpr[idx].Buffer(), exc.what());
	    return false;
    }
    try {
        int dimSamples;
        MDSplus::Data *nodeTimebase = MDSplus::compile(timebaseExpr[idx].Buffer(), tree);
        MDSplus::Data *dataTimebase = tree->tdiData(nodeTimebase);
        timebase = dataTimebase->getDoubleArray(&dimSamples);
        MDSplus::deleteData(dataTimebase);
        MDSplus::deleteData(nodeTimebase);
        if((uint32)dimSamples < numDataSamples)
	    numDataSamples = dimSamples;
	    tDiff = (timebase[dimSamples - 1] - timebase[0])/dimSamples;
	    return true;
    }catch(MDSplus::MdsException &exc)
    {
	    REPORT_ERROR(ErrorManagement::ParametersError, "Cannot read timebase for signal %s: %s", timebaseExpr[idx].Buffer(), exc.what());
	    return false;
    }
}
    
void MDSReaderGAM::notifySignalsEnded()
{
    for(uint32 i = 0; i < nOfMessages; i++)
    {
        if(signalsEndedMsg[i].IsValid())
	    {
            if (!MessageI::SendMessage(signalsEndedMsg[i], this)) {
                StreamString destination = signalsEndedMsg[i]->GetDestination();
                StreamString function = signalsEndedMsg[i]->GetFunction();
                REPORT_ERROR(ErrorManagement::FatalError, "Could not send signalsEnded message to %s [%s]", destination.Buffer(), function.Buffer());
            }
        }
    }
}


CLASS_REGISTER(MDSReaderGAM, "1.0")
}
