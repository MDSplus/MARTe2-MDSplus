/**
 * @file PickSampleGAM.cpp
 * @brief Source file for class ResamplerGAM
 * @date Jul 30, 2019
 * @author nn
 *

 * @details This source file contains the definition of all the methods for the
 * class PickSampleGAM (public, protected, and private). Be aware that some
 * methods, such as those inline could be defined on the header file, instead.
 */

#define DLL_API

/*---------------------------------------------------------------------------*/
/*                         Standard header includes                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                         Project header includes                           */
/*---------------------------------------------------------------------------*/

#include "PickSampleGAM.h"
#include "AdvancedErrorManagement.h"
#include <iostream>

/*---------------------------------------------------------------------------*/
/*                           Static definitions                              */
/*---------------------------------------------------------------------------*/

namespace MARTe {

PickSampleGAM::PickSampleGAM() : GAM(){
    numInSignals = 0;
    numOutSignals = 0;
    signalSamples = NULL_PTR(uint32 *);
    signalByteSize = NULL_PTR(uint32 *);
    inputSignals = NULL_PTR(uint8 **);
    outputSignals = NULL_PTR(uint8 **);

}

PickSampleGAM::~PickSampleGAM() {

    if(signalSamples !=  NULL_PTR(uint32 *))
    {
        GlobalObjectsDatabase::Instance()->GetStandardHeap()->Free(reinterpret_cast<void *&>(signalSamples));
    }    
    if(signalByteSize !=  NULL_PTR(uint32 *))
    {
        GlobalObjectsDatabase::Instance()->GetStandardHeap()->Free(reinterpret_cast<void *&>(signalByteSize));
    }    
    if(inputSignals !=  NULL_PTR(uint8 **))
    {
	GlobalObjectsDatabase::Instance()->GetStandardHeap()->Free(reinterpret_cast<void *&>(inputSignals));
    }
    if(outputSignals !=  NULL_PTR(uint8 **))
    {
	GlobalObjectsDatabase::Instance()->GetStandardHeap()->Free(reinterpret_cast<void *&>(outputSignals));
    }
}

bool PickSampleGAM::Initialise(StructuredDataI & data) {
	
    REPORT_ERROR(ErrorManagement::Debug, "PickSampleGAM: INITIALIZE");
    
    bool ok = GAM::Initialise(data);
    if (!ok) {
            REPORT_ERROR(ErrorManagement::InitialisationError,
                                      "Initialization failed.");
            return ok;
    }
    
    /**
      * Firstly, number of inputs and outputs is read from the configuration file.
      */
    
    ok = data.MoveRelative("InputSignals");
    if(!ok){
            REPORT_ERROR(ErrorManagement::ParametersError,
                                      "Cannot find InputSignals node in configuration file.");
            return ok;
    }
    
    numInSignals = data.GetNumberOfChildren();
    
    // Back to the GAM node
    data.MoveToAncestor(1);
    
    ok = data.MoveRelative("OutputSignals");
    if(!ok){
            REPORT_ERROR(ErrorManagement::ParametersError,
                                      "Cannot find OutputSignals node in configuration file.");
            return ok;
    }
    
    numOutSignals = data.GetNumberOfChildren();
    //Consider also the case of a single output that is going to contain the inputs (first sample) in a structure
    ok = (numOutSignals == 1 || numOutSignals == numInSignals);
    if(!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError,
                                      "The number of input signals shall be aither 1 or equal to the number of output signals");
            return ok;
    }
    // Back to the GAM node
    data.MoveToAncestor(1);
    return ok;
}

bool PickSampleGAM::Setup() {
	
    bool ok;
    
    REPORT_ERROR(ErrorManagement::Debug, "PickSampleGAM SETUP");
    
    /**
      * Firstly, types and dimensions of signals are retrieved and checked.
      */

    numInSignals = GetNumberOfInputSignals();
    numOutSignals = GetNumberOfOutputSignals();

    signalSamples = reinterpret_cast<uint32 *>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(numInSignals * sizeof(uint32)));
    signalByteSize = reinterpret_cast<uint32 *>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(numInSignals * sizeof(uint32)));
    TypeDescriptor signalType;
    uint32 signalElement;
    totSignalByteSize = 0;



    // Inputs
    for (uint32 sigIdx = 0; sigIdx < numInSignals; sigIdx++) {
            
        signalType = GetSignalType(InputSignals, sigIdx);
            
        ok = GetSignalNumberOfSamples(InputSignals, sigIdx, signalSamples[sigIdx]);
        if (!ok) {
                    
            REPORT_ERROR(ErrorManagement::Exception,
                                              "Error in GetSignalNumberOfSamples: input signal %i does not exist.", sigIdx);
            return ok;
                    
        }
        ok = GetSignalByteSize(InputSignals,sigIdx, signalByteSize[sigIdx]);
        if (!ok) {
                    
            REPORT_ERROR(ErrorManagement::Exception,
                                              "Error in GetSignalNumberOfSamples: input signal %i does not exist.", sigIdx);
            return ok;
                    
        }
        totSignalByteSize += signalByteSize[sigIdx];
        ok = GetSignalNumberOfElements(InputSignals,sigIdx, signalElement);
        if (!ok) {
                    
            REPORT_ERROR(ErrorManagement::Exception,
                                              "Error in GetSignalNumberOfElements: input signal %i does not exist.", sigIdx);
            return ok;
                    
        }
        if(numInSignals == numOutSignals) //Check type only if not compacting all inputs in a single output
        {
            TypeDescriptor outType = GetSignalType(OutputSignals, sigIdx);
            ok = (outType == signalType)
                ||(outType == UnsignedInteger32Bit && signalType == SignedInteger32Bit)
                ||(outType == SignedInteger32Bit && signalType == UnsignedInteger32Bit);
            if (!ok) {
                StreamString signalName;
                GetSignalName(InputSignals, sigIdx, signalName);
                REPORT_ERROR(ErrorManagement::Exception,
                                              "Output type %s for signal %s shall be the same of the corresponding input %s.", 
                         TypeDescriptor::GetTypeNameFromTypeDescriptor(signalType), signalName.Buffer(), TypeDescriptor::GetTypeNameFromTypeDescriptor(outType));
                return ok;
            }

            uint32 numOutElements;
            ok = GetSignalNumberOfElements(OutputSignals, sigIdx, numOutElements);
            if (!ok) {
                REPORT_ERROR(ErrorManagement::Exception,
                                                "Error in GetSignalNumberOfElements: output signal %i does not exist.", sigIdx);
                return ok;	
            }
            ok = (numOutElements == signalElement);
            if (!ok) {
                REPORT_ERROR(ErrorManagement::Exception,
                                                "Input/output number of elements different for  signal %i.", sigIdx);
                return ok;  
            }
            uint32 numOutSamples;
            ok = GetSignalNumberOfSamples(OutputSignals, sigIdx, numOutSamples);
            if (!ok) {
                    REPORT_ERROR(ErrorManagement::Exception,
                                            "Error in GetSignalNumberOfSamples: output signal %i does not exist.", sigIdx);
                    return ok;	
            }
            ok = (numOutSamples == 1);
            if (!ok) {
                    
                    REPORT_ERROR(ErrorManagement::Exception,
                                            "Output signal %i shall have one single sample.", sigIdx);
            return ok;
            }
        }
    }
   //handle the separate case in which all inputs (first sample) is compacted into a single output
    if(numInSignals != numOutSignals) //In this case there is a single out signal due to the checks performed at initialization
    {
        uint32 numOutSamples;
        bool ok = GetSignalNumberOfSamples(OutputSignals, 0, numOutSamples);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::Exception,
                    "Error in GetSignalNumberOfSamples: output signal 0 does not exist.");
            return ok;	
        }
        ok = (numOutSamples == 1);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::Exception,
                                            "Output signal 0 shall have one single sample.");
            return ok;
        }
        uint32 outBytes;
        ok = GetSignalByteSize(OutputSignals, 0, outBytes);
        if (!ok) {
           REPORT_ERROR(ErrorManagement::Exception,
                                              "Error in GetSignalNumberOfSamples: output signal 0 does not exist.");
            return ok;
        }
 /*       if(totSignalByteSize != outBytes)
        {           
            REPORT_ERROR(ErrorManagement::Exception,
                                              "The dimension in bytes (%d) of the single output shall be equal to the sum of that of the first sample of all inputs(%d)",
                                              outBytes, totSignalByteSize);
            return false;
        } 
 */   }


    //Allocate inputs, outputs and compute offsets
    inputSignals = reinterpret_cast<uint8 **>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(numInSignals * sizeof(uint8 **)));
    outputSignals = reinterpret_cast<uint8 **>(GlobalObjectsDatabase::Instance()->GetStandardHeap()->Malloc(numOutSignals * sizeof(uint8 **)));


    for (uint32 sigIdx = 0; sigIdx < numInSignals; sigIdx++) 
    {
        inputSignals[sigIdx] = reinterpret_cast<uint8 *>(GetInputSignalMemory(sigIdx));
    }
    for (uint32 sigIdx = 0; sigIdx < numOutSignals; sigIdx++) 
    {
        outputSignals[sigIdx] = reinterpret_cast<uint8 *>(GetOutputSignalMemory(sigIdx));
    }
    
    return ok;
}

bool PickSampleGAM::Execute() {
	
 //   REPORT_ERROR(ErrorManagement::Debug, " PICK SAMPLE EXECUTE");
    if(numInSignals == numOutSignals)
    {
        for (uint32 sigIdx = 0; sigIdx < numInSignals; sigIdx++) {
 //Gabriele Oct 2023: copy last sample, i.e. the most recent one
 //           memcpy(outputSignals[sigIdx], inputSignals[sigIdx], signalByteSize[sigIdx]); //Copy first sample
//            memcpy(outputSignals[sigIdx], inputSignals[sigIdx]+(signalSamples[sigIdx] - 1)*signalByteSize[sigIdx], signalByteSize[sigIdx]); //Copy last sample
            memcpy(outputSignals[sigIdx], inputSignals[sigIdx], signalByteSize[sigIdx]); //Copy first sample
        }
    }
    else //Compacting inputs in a single output
    {
        uint32 byteOffset = 0;
        for (uint32 sigIdx = 0; sigIdx < numInSignals; sigIdx++) {
            memcpy(outputSignals[0]+byteOffset, inputSignals[sigIdx], signalByteSize[sigIdx]); //Copy first sample
            byteOffset += signalByteSize[sigIdx];
        }
    }
   
    return true;
}


CLASS_REGISTER(PickSampleGAM, "1.0")

} /* namespace MARTe */
