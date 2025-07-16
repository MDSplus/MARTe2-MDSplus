/**
 * @file DutyCycleGAM.cpp
 * @brief Source file for class ResamplerGAM
 * @date Jul 30, 2019
 * @author nn
 *

 * @details This source file contains the definition of all the methods for the
 * class DutyCycleGAM (public, protected, and private). Be aware that some
 * methods, such as those inline could be defined on the header file, instead.
 */

#define DLL_API

/*---------------------------------------------------------------------------*/
/*                         Standard header includes                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                         Project header includes                           */
/*---------------------------------------------------------------------------*/

#include "DutyCycleGAM.h"
#include "AdvancedErrorManagement.h"
#include <iostream>

/*---------------------------------------------------------------------------*/
/*                           Static definitions                              */
/*---------------------------------------------------------------------------*/

namespace MARTe {

DutyCycleGAM::DutyCycleGAM() : GAM(){
    enableIdx = 0;
    clockIdx = 0;

    upClocks = 0;
    downClocks = 0;

    inClockCount = 0;
    outClockIsHigh = 0;

    andMask = 0;
    orMask = 0;
}

DutyCycleGAM::~DutyCycleGAM() {}


bool DutyCycleGAM::Initialise(StructuredDataI & data) {
	
	REPORT_ERROR(ErrorManagement::Debug, "DutyCycleGAM: INITIALIZE");
	
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
	
	uint32 numSignals;
	numSignals = data.GetNumberOfChildren();
	ok = (numSignals == 1);
	if(!ok){
		REPORT_ERROR(ErrorManagement::ParametersError,
					 "DutyCycleGAM shall have exactly one input signal.");
		return ok;
	}
	// Back to the GAM node
	data.MoveToAncestor(1);
	
	ok = data.MoveRelative("OutputSignals");
	if(!ok){
		REPORT_ERROR(ErrorManagement::ParametersError,
					 "Cannot find OutputSignals node in configuration file.");
		return ok;
	}
	
	numSignals = data.GetNumberOfChildren();
	ok = (numSignals == 1);
	if(!ok){
		REPORT_ERROR(ErrorManagement::ParametersError,
					 "DutyCycleGAM shall have exactly one output signal.");
		return ok;
	}
	// Back to the GAM node
	data.MoveToAncestor(1);

        ok = data.Read("InFrequency", inFrequency);
	if(!ok) {
		REPORT_ERROR(ErrorManagement::ParametersError,
					 "InFrequency shall be specified");
		return ok;
	}
        ok = data.Read("OutFrequency", outFrequency);
	if(!ok) {
		REPORT_ERROR(ErrorManagement::ParametersError,
					 "OutFrequency shall be specified");
		return ok;
	}
        ok = data.Read("DutyCycle", dutyCycle);
	if(!ok) {
		REPORT_ERROR(ErrorManagement::ParametersError,
					 "DutyCycle shall be specified");
		return ok;
	}
        ok = data.Read("ClockIdx", clockIdx);
	if(!ok) {
		REPORT_ERROR(ErrorManagement::ParametersError,
					 "ClockIdx shall be specified");
		return ok;
	}
        ok = data.Read("EnableIdx", enableIdx);
	if(!ok) {
	    enableIdx = -1;
	}

        ok = data.Read("AndMask", andMask);
	if(!ok) {
	    andMask = 0xffffffff;
	}
        ok = data.Read("OrMask", orMask);
	if(!ok) {
	    orMask = 0;
	}
	return true;
}

bool DutyCycleGAM::Setup() {
	
	bool ok;
	
	REPORT_ERROR(ErrorManagement::Debug, "SETUP");
	uint32 nElements = 0;
	ok = GetSignalNumberOfElements(InputSignals, 0, nElements);
	if (!ok) {
	  REPORT_ERROR(ErrorManagement::Exception,
						 "Error in GetSignalNumberOfElements: input signal  does not exist.");
	  return ok;
			
	}
	ok = (nElements == 1);
	if (!ok) {
	  REPORT_ERROR(ErrorManagement::Exception,
						 "Input signal  shall have exactly one element");
	  return ok;
			
	}
	uint32 nSamples = 0;
	GetSignalNumberOfSamples(InputSignals, 0, nSamples);
	ok = (nSamples == 1);
	if (!ok) {
	  REPORT_ERROR(ErrorManagement::Exception,
						 "Input signal  shall have exactly one sample");
	  return ok;
			
	}
	TypeDescriptor currType = GetSignalType(InputSignals, 0);
	ok = (currType == UnsignedInteger32Bit || currType == SignedInteger32Bit);
	if (!ok) {
	  REPORT_ERROR(ErrorManagement::Exception,
						 "Input signal  be either UnsignedInteger32Bits or SignedInteger32Bits");
	  return ok;
			
	}
	
	ok = GetSignalNumberOfElements(OutputSignals, 0, nElements);
	if (!ok) {
	  REPORT_ERROR(ErrorManagement::Exception,
						 "Error in GetSignalNumberOfElements: output signal  does not exist.");
	  return ok;
			
	}
	ok = (nElements == 1);
	if (!ok) {
	  REPORT_ERROR(ErrorManagement::Exception,
						 "Output signal  shall have exactly one element");
	  return ok;
			
	}
	GetSignalNumberOfSamples(OutputSignals, 0, nSamples);
	ok = (nSamples == 1);
	if (!ok) {
	  REPORT_ERROR(ErrorManagement::Exception,
						 "Output signal  shall have exactly one sample");
	  return ok;
			
	}
	currType = GetSignalType(OutputSignals, 0);
	ok = (currType == UnsignedInteger32Bit || currType == SignedInteger32Bit);
	if (!ok) {
	  REPORT_ERROR(ErrorManagement::Exception,
						 "Output signal  be either UnsignedInteger32Bits or SignedInteger32Bits");
	  return ok;
			
	}
	inBits = reinterpret_cast<uint32 *>(GetInputSignalMemory(0));
	outBits = reinterpret_cast<uint32 *>(GetOutputSignalMemory(0));
	*outBits = 0;
	outClockIsHigh = false;
	
	dutyCycle /= 100.;
	int32 clockCount = (int32)(inFrequency/outFrequency + 0.5);
    	upClocks = (uint32)(clockCount * dutyCycle + 0.5);
    	downClocks = clockCount - upClocks;    
	inClockCount = 0;
	return ok;
}

bool DutyCycleGAM::Execute() {
        uint8  enabledIn;
	if(enableIdx >= 0) {
	    enabledIn = (*inBits & (0x01 << enableIdx)) != 0;
	} else  {
	    enabledIn = 1;
	}
	uint32 clockOut;
        if(upClocks == 0)
            clockOut = 0;
	else if (downClocks == 0)
	    clockOut = 1;
        else
        {
	    if (outClockIsHigh)
	    {
		clockOut = 1;
		inClockCount++;
		if(inClockCount >= upClocks)    
		{
		    clockOut = 0;
		    inClockCount = 0;
		    outClockIsHigh = false;
		}
	    }
	    else
	    {
		clockOut = 0;
		inClockCount++;
		if(inClockCount >= downClocks)
		{
		    clockOut = 1;
		    inClockCount = 0;
		    outClockIsHigh = true;
		}
	    }
	}
	if(!enabledIn) {
	    clockOut = 0;
	    outClockIsHigh = false;
	}	
	    
	*outBits = (((*inBits & andMask) | orMask) & ~(1 << clockIdx)) | (clockOut << clockIdx); 
	return true;
    }


CLASS_REGISTER(DutyCycleGAM, "1.0")

} /* namespace MARTe */
