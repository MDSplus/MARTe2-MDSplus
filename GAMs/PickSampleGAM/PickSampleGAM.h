/**
 * @file PickSampleGAM.h
 * @brief Header file for class ConstantGAM
 * @date Nov 9, 2016 TODO Verify the value and format of the date
 * @author aneto TODO Verify the name and format of the author
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

 * @details This header file contains the declaration of the class ConstantGAM
 * with all of its public, protected and private members. It may also include
 * definitions for inline methods which need to be visible to the compiler.
 */

#ifndef PickSampleGAM_H_
#define PickSampleGAM_H_

/*---------------------------------------------------------------------------*/
/*                        Standard header includes                           */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                        Project header includes                            */
/*---------------------------------------------------------------------------*/
#include "GAM.h"
#include "StructuredDataI.h"
#include "MessageI.h"
/*---------------------------------------------------------------------------*/
/*                           Class declaration                               */
/*---------------------------------------------------------------------------*/
/**************************************************************
PickSampleGAM  will pick the last sample for an arbitrary number of inputs,
where every input is an arbitrary set of samples for scalars or array of arbitrary length. 
Supported types are float32, float64, int8, int16, int32, int64.
The GAM expects an equal number of outputs and every output will be represented by the same type and dimension 
of the input, but composed of a single sample. If the input consists of a single sample, 
output shall be the same as input.

****************************************************************/
namespace MARTe {
class PickSampleGAM : public GAM {
public:
    CLASS_REGISTER_DECLARATION()

    PickSampleGAM();

    virtual ~PickSampleGAM();

    virtual bool Setup();

    virtual bool Execute();

    virtual bool Initialise(StructuredDataI & data);

    uint32 numInSignals;
    uint32 numOutSignals;
    uint32 totSignalByteSize;
    uint32* signalByteSize;
    uint32* signalSamples;

    uint8 **inputSignals;
    uint8 **outputSignals;
    };
}

/*---------------------------------------------------------------------------*/
/*                        Inline method definitions                          */
/*---------------------------------------------------------------------------*/

#endif 
	
