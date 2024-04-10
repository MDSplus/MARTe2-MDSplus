/**
 * @file PyGAM.cpp
 * @brief Source file for class PyGAM
 * @date Oct 8, 2019
 * @author nn
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
 *
 * @details This source file contains the definition of all the methods for
 * the class PyGAM (public, protected, and private). Be aware that some
 * methods, such as those inline could be defined on the header file, instead.
 * 
 */

#define DLL_API

/*---------------------------------------------------------------------------*/
/*                         Standard header includes                          */
/*---------------------------------------------------------------------------*/

#include <iostream>

/*---------------------------------------------------------------------------*/
/*                         Project header includes                           */
/*---------------------------------------------------------------------------*/

#include "PyGAM.h"
#include "AdvancedErrorManagement.h"
#include <signal.h>

/*---------------------------------------------------------------------------*/
/*                           Static definitions                              */
/*---------------------------------------------------------------------------*/

namespace MARTe {

PyGAM::PyGAM() : GAM(){
	
}

PyGAM::~PyGAM() {
	
	Py_FinalizeEx();
	
}

// Debug method to print objects.
static void PyPrint(PyObject *obj) {
	
	PyObject* repr = PyObject_Repr(obj);
	PyObject* str = PyUnicode_AsEncodedString(repr, "utf-8", "~E~");
	const char *bytes = PyBytes_AS_STRING(str);
	
	REPORT_ERROR_STATIC(ErrorManagement::Debug, "OBJECT REPRINTED: %s", bytes);
	
	Py_XDECREF(repr);
	Py_XDECREF(str);
}



void sig_handler(int signo)
{
	if (signo == SIGINT) {
		printf("received SIGINT\n");
	exit(0);
	
	}
}

bool PyGAM::Initialise(StructuredDataI & data) {

 	// Runs the Python initialize() function.
  
	bool ok = GAM::Initialise(data);
	
	// These are needed later
	PyObject* paramValues;
	AnyType arrayDescription;
	
	signal(SIGINT, sig_handler);
	
	// Debug
	isFirstRun = true;
	REPORT_ERROR(ErrorManagement::Debug, "INITIALISE");
	
	// Debug sleep time
	ok = data.Read("SleepTime", sleepTime);
	if (!ok) {
		REPORT_ERROR(ErrorManagement::Debug,
					 "Sleep time not set. By default it will be zeroed.");
		sleepTime = 0;
	}
	
	ok = data.Read("RealtimeOutputCheck", realtimeCheckFlag);
	bool paramOk = (realtimeCheckFlag == 0 || realtimeCheckFlag == 1);		// realtimeCheckFlag can be either 0 or 1.
	if (!ok || !paramOk) {
		REPORT_ERROR(ErrorManagement::Debug,
					 "RealtimeCheck not set (or incorrect format). By default it will be set to 1.");
		realtimeCheckFlag = 1;
	}
	
	ok = data.Read("FileName", fileName);
	if (!ok) {
		REPORT_ERROR(ErrorManagement::ParametersError,
					 "Error while reading FileName leaf from configuration file. Was it defined?");
		return ok;
	}
	
 	pModuleName = PyUnicode_FromString(fileName.Buffer());
	
	// Loading Python interpreter.
	Py_InitializeEx(0);
	
	// Loading numpy module.
	import_array();
	
	/***********************************************************************//**
	* 
	* Firstly, the module (the .py file) is loaded by the Python interpreter.
	* 
	***************************************************************************/
	
	pName = PyUnicode_FromString(fileName.Buffer());
	// Error checking of pName left out
	
	pModule = PyImport_Import(pName);
	Py_DECREF(pName);
	
	ok = (pModule != NULL);
	if (!ok) {
		REPORT_ERROR(ErrorManagement::Exception,
					 "Failed to load %s.",
			   fileName.Buffer());
		goto error;
	}
	
	pName = PyUnicode_FromString("pygam");
	pygamModule = PyImport_Import(pName);
	Py_DECREF(pName);
	
	ok = (pygamModule != NULL);
	if (!ok) {
		REPORT_ERROR(ErrorManagement::Exception,
					 "Failed to load pygam.");
		goto error;
	}
	
	/***********************************************************************//**
	* 
	* Data on the code are retrieved using APIs from the Python module and
	* stored in a PyArgumentStruct.
	* 
	***************************************************************************/
	
	// PyObject_GetAttrString() gets a handle to the object,
	// then the object is checked to verify if it is callable,
	// PyObject_CallObject calls the function
	// and the output value pValue is assigned accordingly.
	
	pFunc = PyObject_GetAttrString(pygamModule, "initialize");
	
	ok = (pFunc && PyCallable_Check(pFunc));
	if (!ok) {
		REPORT_ERROR(ErrorManagement::Exception,
					 "Python initialize() function is not callable.");
		goto error;
	}
	
	
	pValue = PyObject_CallFunctionObjArgs(pFunc, pModuleName, NULL);
	ok = (pValue != NULL);
	if (!ok) {
		REPORT_ERROR(ErrorManagement::Exception,
					 "Failed to call Python initialize function.");
		goto error;
	}
	
	// Number of inputs.
	pFunc = PyObject_GetAttrString(pygamModule, "getNumberOfInputs");
	
	ok = (pFunc && PyCallable_Check(pFunc));
	if (!ok) goto error;
	
//	pValue = PyObject_CallObject(pFunc, NULL);

	pValue = PyObject_CallFunctionObjArgs(pFunc, pModuleName, NULL);


	ok = (pValue != NULL);
	if (!ok) goto error;
	
	pyNumOfInputs = PyLong_AsLong(pValue);
	
	// Number of outputs.
	pFunc = PyObject_GetAttrString(pygamModule, "getNumberOfOutputs");
	
	ok = (pFunc && PyCallable_Check(pFunc));
	if (!ok) goto error;
	
//	pValue = PyObject_CallObject(pFunc, pModuleName, NULL);
	pValue = PyObject_CallFunctionObjArgs(pFunc, pModuleName, NULL);

	ok = (pValue != NULL);
	if (!ok) goto error;
	
	pyNumOfOutputs = PyLong_AsLong(pValue);
	
	// Number of parameters.
	pFunc = PyObject_GetAttrString(pygamModule, "getNumberOfParameters");
	
	ok = (pFunc && PyCallable_Check(pFunc));
	if (!ok) goto error;
	
//	pValue = PyObject_CallObject(pFunc, NULL);
	pValue = PyObject_CallFunctionObjArgs(pFunc, pModuleName, NULL);
	ok = (pValue != NULL);
	if (!ok) goto error;
	
	pyNumOfParameters = PyLong_AsLong(pValue);
	
	// Now that no. of inputs and outputs are known structures can be allocated.
	pyInputStruct     = new PyArgumentStruct[pyNumOfInputs];
	pyOutputStruct    = new PyArgumentStruct[pyNumOfOutputs];
	pyParameterStruct = new PyArgumentStruct[pyNumOfParameters];
	
	// Argument datatypes
	ok = pyInputStruct->GetTypes(PyArgInputs, pyNumOfInputs, pygamModule, pModuleName);
	if (ok) ok = pyOutputStruct->GetTypes(PyArgOutputs, pyNumOfOutputs, pygamModule, pModuleName);
	if (ok) ok = pyParameterStruct->GetTypes(PyArgParameters, pyNumOfParameters, pygamModule, pModuleName);
	if (!ok) {
		REPORT_ERROR(ErrorManagement::Exception,
					 "GetTypes failed.");
		return ok;
	}
	
	// Argument dimensions
	ok = pyInputStruct->GetDimensions(PyArgInputs, pyNumOfInputs, pygamModule, pModuleName);
	if (ok) ok = pyOutputStruct->GetDimensions(PyArgOutputs, pyNumOfOutputs, pygamModule, pModuleName);
	if (ok) ok = pyParameterStruct->GetDimensions(PyArgParameters, pyNumOfParameters, pygamModule, pModuleName);
	if (!ok) {
		REPORT_ERROR(ErrorManagement::Exception,
					 "GetDimensions failed.");
		return ok;
	}
	
	// Argument names
	ok = pyInputStruct->GetNames(PyArgInputs, pyNumOfInputs, pygamModule, pModuleName);
	if (ok) ok = pyOutputStruct->GetNames(PyArgOutputs, pyNumOfOutputs, pygamModule, pModuleName);
	if (ok) ok = pyParameterStruct->GetNames(PyArgParameters, pyNumOfParameters, pygamModule, pModuleName);
	if (!ok) {
		REPORT_ERROR(ErrorManagement::Exception,
					 "GetNames failed.");
		return ok;
	}
	
	/***********************************************************************//**
	* 
	* Parameter values are retrieved from the configuration file.
	* 
	***************************************************************************/
	
	// This is the dictionary where parameter data is stored in the Python code.
	// By updating it parameters are passed from MARTe2 configuration file
	// to the Python code.
	paramValues = PyObject_GetAttrString(pygamModule, "data");
	ok = (paramValues != NULL);
	if (!ok) {
		REPORT_ERROR(ErrorManagement::Exception,
					 "Failed to load ParameterDict.");
		goto error;
	}
	
	// Dimensions of the parameter
	uint32 numberOfDimensions;
	uint32 numberOfRows;
	uint32 numberOfCols;
	
	ok = data.MoveRelative("Parameters");
	if(!ok){
		REPORT_ERROR(ErrorManagement::ParametersError,
						"Cannot find Parameters node in configuration file.");
		return ok;
	}
	
	for (uint32 paramIdx = 0; paramIdx < pyNumOfParameters; paramIdx++) {
		
		const char8* paramName = (pyParameterStruct[paramIdx].name).Buffer();
		
		std::cout << paramName << "\n";
		
		// Before we store the parameter, we must know how much memory to allocate
		arrayDescription = data.GetType(paramName);
		ok = arrayDescription.GetDataPointer() != NULL_PTR(void *);
		if (!ok) {
			REPORT_ERROR(ErrorManagement::ParametersError,
						 "Error while reading param. %s from configuration file. GetDataPointer() == NULL_PTR(void *). Was the parameter defined?",
						 paramName);
			return ok;
		}
		
		/// Since we are at it, various coherence checks between model and configuration file are performed
		numberOfDimensions = arrayDescription.GetNumberOfDimensions();
		
		ok = (numberOfDimensions <= 2u);	// More than 2 dimensions is not supported
		if (!ok) {
			
			REPORT_ERROR(ErrorManagement::ParametersError,
						 "Param. %i (%s) has %i dimensions. Dimensions must be less or equal to 2",
						 paramIdx, paramName, numberOfDimensions);
			return ok;
			
		}
		
		ok = (numberOfDimensions == pyParameterStruct[paramIdx].numberOfDimensions);
		if (!ok) {
			
			REPORT_ERROR(ErrorManagement::ParametersError,
						 "Param. %i (%s) dimension mismatch. Model: %i, configuration file: %i",
						 paramIdx,
						 paramName,
						 pyParameterStruct[paramIdx].numberOfDimensions,
						 numberOfDimensions);
			return ok;
			
		}
		
		numberOfRows = arrayDescription.GetNumberOfElements(1u);
		numberOfCols = arrayDescription.GetNumberOfElements(0u);
		
		ok = (numberOfRows == pyParameterStruct[paramIdx].dimensions[0]);
		if (!ok) {
			
			REPORT_ERROR(ErrorManagement::ParametersError,
						 "Param. %i (%s) nOfRows mismatch. Model: %i, configuration file: %i",
						 paramIdx,
						 paramName,
						 pyParameterStruct[paramIdx].dimensions[0],
						 numberOfRows);
			return ok;
			
		}
		
		ok = (numberOfCols == pyParameterStruct[paramIdx].dimensions[1]);
		if (!ok) {
			
			REPORT_ERROR(ErrorManagement::ParametersError,
						 "Param. %i (%s) nOfCols mismatch. Model: %i, configuration file: %i",
						 paramIdx,
						 paramName,
						 pyParameterStruct[paramIdx].dimensions[1],
						 numberOfCols);
			return ok;
			
		}
		
		// Now parameter values are read from the configuration file.
		// Then, parameters are updated accordingly.
		// (This part does not need to be templated since parameters are read
		// only from config. file and can then be safely assumed to be all float64)
		switch (numberOfDimensions) {
			
			case 0: {
				
				float64 configValue;
				ok = data.Read(paramName, configValue);
				
				if (!ok) {
					REPORT_ERROR(ErrorManagement::ParametersError,
								 "Error while reading param. %s from configuration file. Was it defined?",
								 paramName);
								 return ok;
				}
				
				PyDict_SetItemString(paramValues, paramName, PyFloat_FromDouble(configValue));
				
				//std::cout << configValue << std::endl;
				
				break;
				
			}
			
			case 1: {
				
				Vector<float64> configVector(numberOfRows*numberOfCols);
				ok = data.Read(paramName, configVector);
				if (!ok) {
					REPORT_ERROR(ErrorManagement::ParametersError,
								 "Error while reading param. %s from configuration file. Was it defined?",
								 paramName);
								 return ok;
				}
				
				PyObject* configList = PyList_New(numberOfRows*numberOfCols);
				
				for (uint32 j = 0; j < numberOfRows*numberOfCols; j++) {
					
					PyList_SetItem(configList, j, PyFloat_FromDouble(configVector[j]));
					
					//std::cout << configVector[j] << std::endl;
					
				}
				
				PyDict_SetItemString(paramValues, paramName, configList);
				
				break;
				
			}
			
			case 2: {
				
				Matrix<float64> configMatrix(numberOfRows, numberOfCols);
				ok = data.Read(paramName, configMatrix);
				if (!ok) {
					REPORT_ERROR(ErrorManagement::ParametersError,
								 "Error while reading param. %s from configuration file. Was it defined?",
								 paramName);
					return ok;
				}
				
				PyObject* configList = PyList_New(numberOfRows);
				
				for (uint32 j = 0; j < numberOfRows; j++) {
					
					PyObject* rowList = PyList_New(numberOfCols);
					
					for (uint32 k = 0; k < numberOfCols; k++) {
						
						PyList_SetItem(rowList, k, PyFloat_FromDouble(configMatrix[j][k]));
						
						//std::cout << configMatrix[j][k] << std::endl;
						
					}
					
					PyList_SetItem(configList, j, rowList);
					
				}
				
				PyDict_SetItemString(paramValues, paramName, configList);
				
				break;
				
			}
			
			default:
				REPORT_ERROR(ErrorManagement::ParametersError,
							 "Param. %s has %i dimension(s). Dimensions must be less or equal to 2",
							 paramName, numberOfDimensions);
				return ok;
				break;
				
		}
		
	}
	
	ok = data.MoveToAncestor(1);
	if(!ok){
		REPORT_ERROR(ErrorManagement::InitialisationError,
						"Failed MoveToAncestor() from %s", data.GetName());
		return ok;
	}
	
	PyPrint(paramValues);
	
	// Call the setup() function.
	pFunc = PyObject_GetAttrString(pModule, "setup");
	
	ok = (pFunc && PyCallable_Check(pFunc));
	if (!ok) {
		REPORT_ERROR(ErrorManagement::Exception,
					 "Python setup() function is not callable.");
		goto error;
	}
	
	pValue = PyObject_CallFunctionObjArgs(pFunc, NULL);
	ok = (pValue != NULL);
	if (!ok) {
		REPORT_ERROR(ErrorManagement::Exception,
					 "Failed to call Python setup() function.");
		goto error;
	}
	
	// Check if the execute function is callable.
	pFunc = PyObject_GetAttrString(pModule, "execute");
	ok = (pFunc && PyCallable_Check(pFunc));
	if (!ok) {
		REPORT_ERROR(ErrorManagement::Exception,
					 "Python execute() function is not callable.");
		goto error;
	}
	
	Py_DECREF(pFunc);
	
	return ok;
	
	/***********************************************************************//**
	* 
	* Error handling.
	* 
	***************************************************************************/
	
	error:
		// Retrieve informations on the error.
		PyErr_Fetch(&pErrorType, &pErrorValue, &pTraceback);
		
		PyObject* pStr = PyObject_Str(pErrorValue);
		
		REPORT_ERROR(ErrorManagement::Exception,
					"PyErr reported: %s",
					 PyUnicode_AsUTF8(pStr)
					);
		
		Py_DECREF(pStr);
		Py_XDECREF(pFunc);
		Py_XDECREF(pModule);
		
		Py_FinalizeEx();
		
		return ok;
	
} // Initialise()

bool PyGAM::Setup() {
	
	// Debug
	REPORT_ERROR(ErrorManagement::Debug, " PYGAM SETUP");
	
	bool ok = false;
	
	/***********************************************************************//**
	* 
	* 1. Coherency checks between data provided in MARTe2 configuration file
	*    and what is written in the Python code.
	* 
	***************************************************************************/
	
	ok = (GetNumberOfInputSignals() == pyNumOfInputs);
	if (!ok) {
		REPORT_ERROR(ErrorManagement::ParametersError,
					 "Number of inputs mismatch. Python: (%i), GAM: (%i)",
					 pyNumOfInputs, GetNumberOfInputSignals()
					);
		return ok;
	}
	
	ok = (GetNumberOfOutputSignals() == pyNumOfOutputs);
	if (!ok) {
		REPORT_ERROR(ErrorManagement::ParametersError,
					 "Number of outputs mismatch. Python: (%i), GAM: (%i)",
					 pyNumOfOutputs, GetNumberOfOutputSignals()
		);
		return ok;
	}
	
	uint32 GAMNumberOfDimensions;
	uint32 GAMNumberOfElements;
	uint32 pyNumberOfElements;
	
	// Cycle over all inputs to retrieve some data from configured database
	// and to perform some coherency check.
	for (uint32 inputIdx = 0; inputIdx < pyNumOfInputs; inputIdx++) {
		
		pyInputStruct[inputIdx].GAMAddress = GetInputSignalMemory(inputIdx);
		pyInputStruct[inputIdx].GAMType    = GetSignalType(InputSignals, inputIdx);
		pyInputStruct[inputIdx].enumType   = NumPyEnumTypeFromMARTe2Type(pyInputStruct[inputIdx].GAMType);
		
		// Dimension check
		ok = GetSignalNumberOfDimensions(InputSignals, inputIdx, GAMNumberOfDimensions);
		if (!ok) {
			REPORT_ERROR(ErrorManagement::ParametersError,
						 "GetSignalNumberOfDimensions() returned false.");
			return ok;
		}
		
		ok = (pyInputStruct[inputIdx].numberOfDimensions == GAMNumberOfDimensions);
		if (!ok) {
			REPORT_ERROR(ErrorManagement::ParametersError,
						 "Module \"%s\", input no. (%i): number of dimension mismatch. Python: (%i), GAM: (%i)",
						 fileName.Buffer(),
						 inputIdx,
						 pyInputStruct[inputIdx].numberOfDimensions,
						 GAMNumberOfDimensions
						);
			return ok;
		}
		
		// Number of element check
		pyNumberOfElements = pyInputStruct[inputIdx].dimensions[0] * pyInputStruct[inputIdx].dimensions[1];
		GetSignalNumberOfElements(InputSignals, inputIdx, GAMNumberOfElements);
		
		pyInputStruct[inputIdx].GAMNumOfElts = GAMNumberOfElements;
		
		ok = (pyNumberOfElements == GAMNumberOfElements);
		if (!ok) {
			REPORT_ERROR(ErrorManagement::ParametersError,
						 "Input no. (%i): number of element mismatch. Python: (%s), GAM: (%s)",
						 inputIdx,
						 pyNumberOfElements,
						 GAMNumberOfElements);
			return ok;
		}
		
		// Datatype check
		ok = (pyInputStruct[inputIdx].pyTypeInMARTe2 == pyInputStruct[inputIdx].GAMType);
		
		if (!ok) {
			REPORT_ERROR(ErrorManagement::ParametersError,
						 "Input no. (%i): type mismatch. Python: (%s), GAM: (%s)",
						 inputIdx,
						 TypeDescriptor::GetTypeNameFromTypeDescriptor(pyInputStruct[inputIdx].pyTypeInMARTe2),
						 TypeDescriptor::GetTypeNameFromTypeDescriptor(pyInputStruct[inputIdx].GAMType)
						);
			return ok;
		}
		
	}
	
	// Cycle over all outputs to retrieve some data from configured database
	// and to perform some coherency check.
	for (uint32 outputIdx = 0; outputIdx < pyNumOfOutputs; outputIdx++) {
		
		pyOutputStruct[outputIdx].GAMAddress = GetOutputSignalMemory(outputIdx);
		pyOutputStruct[outputIdx].GAMType    = GetSignalType(OutputSignals, outputIdx);
		pyOutputStruct[outputIdx].enumType   = NumPyEnumTypeFromMARTe2Type(pyOutputStruct[outputIdx].GAMType);
		
		// Dimension check
		ok = GetSignalNumberOfDimensions(OutputSignals, outputIdx, GAMNumberOfDimensions);
		if (!ok) {
			REPORT_ERROR(ErrorManagement::ParametersError,
						 "GetSignalNumberOfDimensions() returned false.");
			return ok;
		}
		
		ok = (pyOutputStruct[outputIdx].numberOfDimensions==GAMNumberOfDimensions);
		if (!ok) {
			REPORT_ERROR(ErrorManagement::ParametersError,
						 "Module \"%s\", output no. (%i): dimension mismatch. Python: (%i), GAM: (%i)",
						 fileName.Buffer(),
						 outputIdx,
						 pyOutputStruct[outputIdx].numberOfDimensions,
						 GAMNumberOfDimensions
			);
			return ok;
		}
		
		// Number of element check
		pyNumberOfElements = pyOutputStruct[outputIdx].dimensions[0] * pyOutputStruct[outputIdx].dimensions[1];
		GetSignalNumberOfElements(OutputSignals, outputIdx, GAMNumberOfElements);
		
		pyOutputStruct[outputIdx].GAMNumOfElts = GAMNumberOfElements;
		
		ok = (pyNumberOfElements == GAMNumberOfElements);
		if (!ok) {
			REPORT_ERROR(ErrorManagement::ParametersError,
						 "Output no. (%i): number of element mismatch. Python: (%s), GAM: (%s)",
						 outputIdx,
						 pyNumberOfElements,
						 GAMNumberOfElements);
			return ok;
		}
		
		// Datatype check
		ok = (pyOutputStruct[outputIdx].pyTypeInMARTe2 == pyOutputStruct[outputIdx].GAMType);
		
		if (!ok) {
			REPORT_ERROR(ErrorManagement::ParametersError,
						 "Output no. (%i): type mismatch. Python: (%s), GAM: (%s)",
						 outputIdx,
						 TypeDescriptor::GetTypeNameFromTypeDescriptor(pyOutputStruct[outputIdx].pyTypeInMARTe2),
						 TypeDescriptor::GetTypeNameFromTypeDescriptor(pyOutputStruct[outputIdx].GAMType)
			);
			return ok;
		}
		
	}
	
	/***********************************************************************//**
	* 
	* 2. Retrieved informations are used to build PyObject tuples
	*    that will contain actual input and output data.
	*    The tuple will then be used to pass arguments to the execute() function
	*    and to store its outputs.
	* 
	***************************************************************************/
	
	pInputs = CreateArgTuple(InputSignals, pyInputStruct);
	ok = (pInputs!=NULL);
	if (!ok) {
		REPORT_ERROR(ErrorManagement::Exception, "Failed to create input tuple.");
		goto error;
	}
	
	pOutputs = CreateArgTuple(OutputSignals, pyOutputStruct);
	if (!pOutputs) {
		REPORT_ERROR(ErrorManagement::Exception, "Failed to create output tuple.");
		goto error;
	}
	
	PyPrint(pInputs);
	PyPrint(pOutputs);
	
	return ok;
	
	/***********************************************************************//**
	* 
	* 3. Error handling.
	* 
	***************************************************************************/
	
	error:
		// Retrieve informations on the error.
		PyErr_Fetch(&pErrorType, &pErrorValue, &pTraceback);
		
		PyObject* pStr = PyObject_Str(pErrorValue);
		
		REPORT_ERROR(ErrorManagement::Exception,
					 "PyErr reported: %s",
					 PyUnicode_AsUTF8(pStr)
					);
		
		Py_DECREF(pStr);
		Py_XDECREF(pFunc);
		Py_XDECREF(pModule);
		
		Py_FinalizeEx();
		
		return ok;
	
} // Setup()

bool PyGAM::Execute() {
	
	bool ok = false;
	
	// Sleep for debugging
	if (sleepTime!=0) {
		
		MARTe::Sleep::Sec(sleepTime);
		
	}
	
	// Debug
	if (isFirstRun == true) {
		
		REPORT_ERROR(ErrorManagement::Debug, "EXECUTE");
		isFirstRun = false;
		
	}
	
	/***********************************************************************//**
	* 
	* 1. Copy data from GAM memory to the input tuple
	*    (for now this is not needed).
	* 
	***************************************************************************/
	
	// This is not needed, since input tuple is told to map GAM input memory and
	// its reference count never changes afterwards, so its memory address remains the same.
	
	/***********************************************************************//**
	* 
	* 2. Call of the execute() method of the Python code.
	* 
	***************************************************************************/
	
	pOutputs = PyObject_CallObject(pFunc, pInputs);
	
	// TODO: this checks should be moved away from the execute method (maybe a phony execute() call in Setup()?)
	ok = (pOutputs != NULL);
	if (!ok) {
		
		REPORT_ERROR(ErrorManagement::Exception,
					 "pOutputs == NULL. Call failed.");
		
		goto error;
		
	}
	
	ok = PyTuple_Check(pOutputs);
	if (!ok) {
		
		REPORT_ERROR(ErrorManagement::Exception,
					 "Python output is not a tuple.");
		goto error;
		
	}
	
	/***********************************************************************//**
	* 
	* 3. Check that the output tuple has the correct dimensions
	*    and is made of the correct type.
	* 
	***************************************************************************/
	
	if (realtimeCheckFlag == 1) {
		
		uint32 realtimeNumberOfOutputs = PyTuple_Size(pOutputs);
		ok = (realtimeNumberOfOutputs == pyNumOfOutputs);
		if (!ok) {
			REPORT_ERROR(ErrorManagement::Exception,
						 "Unexpected number of outputs from Python execute() function. GAM expected: (%i), Python returned: (%i).",
						 pyNumOfOutputs,
						realtimeNumberOfOutputs
						);
			return ok; 
		}
		
		for (uint32 outputIdx = 0; outputIdx < pyNumOfOutputs; outputIdx++) {
			
			// Get the type of the current output.
			uint32 outputType = PyArray_TYPE((PyArrayObject*) PyTuple_GetItem(pOutputs, outputIdx));
			
			// Compare with the stored type (whose coherence was already checked
			// both against MARTe2 configuration file and Python).
			ok = (outputType == pyOutputStruct[outputIdx].enumType);
			
			// Error logging.
			if (!ok) {
				
				// Generate a string from the numpy type.
				PyObject* objRepresentation = PyObject_Repr((PyObject*) PyArray_DescrFromType(outputType));
				PyObject* str = PyUnicode_AsEncodedString(objRepresentation, "utf-8", "~E~");
				const char *typeBytes = PyBytes_AS_STRING(str);
				
				// Generate a string from MARTe2 type.
				StreamString typeStr = TypeDescriptor::GetTypeNameFromTypeDescriptor(pyOutputStruct[outputIdx].GAMType);
				
				REPORT_ERROR(ErrorManagement::Exception,
							"Output no. %i: unexpected output type from Python execute() function. GAM expected: (%s), Python returned: (%s).",
							outputIdx,
							typeStr.Buffer(),
							typeBytes
							);
				
				REPORT_ERROR(ErrorManagement::Information,
							"NumPy always upcasts the result of an operation to float64 when an integer is involved.");
				
				return ok;
			}
			
			// Get number of elements of the current output.
			uint32 pyNumberOfElements = PyArray_SIZE((PyArrayObject*) PyTuple_GetItem(pOutputs, outputIdx));
			ok = (pyNumberOfElements == pyOutputStruct[outputIdx].GAMNumOfElts);
			
			if (!ok) {
				
				REPORT_ERROR(ErrorManagement::Exception,
							 "Output no. %i: unexpected output number of elements from Python execute() function. GAM expected: (%i), Python returned: (%i).",
							 outputIdx,
							 pyOutputStruct[outputIdx].GAMNumOfElts,
							 pyNumberOfElements
				);
				
			}
			
		}
		
	}
	
	/***********************************************************************//**
	* 
	* 4. Copy data from the output tuple to the GAM memory.
	* 
	***************************************************************************/

 	RefreshData(OutputSignals);
		
	Py_XDECREF(pOutputs);
	
	/***********************************************************************//**
	* 
	* 5. Return and error handling.
	* 
	***************************************************************************/
	
	// If everything went fine.
	return ok;
	
	// Else.
	error:
		// Retrieve informations on the error.
		PyErr_Fetch(&pErrorType, &pErrorValue, &pTraceback);
		
		PyObject* pStr = PyObject_Str(pErrorValue);
		
		REPORT_ERROR(ErrorManagement::Exception,
					"PyErr reported: %s",
				PyUnicode_AsUTF8(pStr)
		);
		
		Py_DECREF(pStr);
		Py_XDECREF(pFunc);
		Py_XDECREF(pModule);
		
		Py_FinalizeEx();
		
		return ok;
	
} // Execute()

PyObject* PyGAM::CreateArgTuple(const SignalDirection direction, PyArgumentStruct* pyStruct) {
	
	bool ok = false;
	
	PyObject* ArgTuple  = NULL;
	uint32    numberOfArgs = 0;
	
	switch (direction) {
		
		case InputSignals:
			
			numberOfArgs = pyNumOfInputs;
			
			break;
		
		case OutputSignals:
			
			numberOfArgs = pyNumOfOutputs;
			
			break;
		
		default:
			
			REPORT_ERROR(ErrorManagement::Exception,
						 "Signal direction not specified.");
			return NULL;
		
	}
	
	ArgTuple = PyTuple_New(numberOfArgs);
	
	// The tuple will have an item for each input argument or return value.
	// We presume that checks have already been carried out, so data in 
	// configuration file matches the requests from the Python code.
	// If that's the case, we can safely use configuration file data to create
	// input and output tuples.
	for (uint32 argIdx = 0; argIdx < numberOfArgs; argIdx++) {
		
		uint32         argNumOfDims;
		uint32         argNumOfElements;
		TypeDescriptor argType;
		
		PyObject*      myArray = NULL;
		npy_intp       dims[2];
		
		ok = GetSignalNumberOfDimensions(direction, argIdx, argNumOfDims);
		if (!ok) return NULL;
		
		ok = GetSignalNumberOfElements(direction, argIdx, argNumOfElements);
		if (!ok) return NULL;
		
		argType = GetSignalType(direction, argIdx);
		
		switch (argNumOfDims) {
			
			case 0:
				
				dims[0] = 1;
				dims[1] = 1;
				
				break;
				
			case 1:
				
				dims[0] = argNumOfElements;
				dims[1] = 1;
				
				break;
				
			case 2:
				
				dims[0] = argNumOfElements;
				dims[1] = 1;
				
				break;
		}
		
		if (direction==InputSignals) {
			
			// Input tuple is created so that its data section is pointing to the memory of the GAM.
			myArray = PyArray_SimpleNewFromData(2, dims, pyStruct[argIdx].enumType, GetInputSignalMemory(argIdx));
			
		} else if (direction==OutputSignals) {
			
			// For the output tuple this unuseful, since every time it is   
			// updated by CallObject() its memory location is changed.
			myArray =  PyArray_SimpleNew(2, dims, pyStruct[argIdx].enumType);
			
		}
		
		// The array is set into the tuple (myArray is not accessible from here on).
		PyTuple_SetItem(ArgTuple, argIdx, myArray);
		
	}
	
	return ArgTuple;
	
} // CreateArgTuple()

bool PyGAM::RefreshData(const SignalDirection direction) {
	
	bool ok = true;
	
	uint32    numberOfArgs = 0;
	
	PyObject* argTuple = NULL;
	
	PyArgumentStruct* pyStruct;
	
	TypeDescriptor argType;
  
        uint32 enumType;
	
	switch (direction) {
		
		case InputSignals:
			
			numberOfArgs = pyNumOfInputs;
			argTuple     = pInputs;
			pyStruct     = pyInputStruct;
			
			break;
			
		case OutputSignals:
			
			numberOfArgs = pyNumOfOutputs;
			argTuple     = pOutputs;
			pyStruct     = pyOutputStruct;
			
			break;
			
		default:
			
			REPORT_ERROR(ErrorManagement::Exception,
						 "Unsupported data direction.");
			return false;
		
	}
	
	for (uint32 argIdx = 0; argIdx < numberOfArgs; argIdx++) {
		
		PyObject* temp = PyTuple_GetItem(argTuple, argIdx);
		
		//argType = GetSignalType(direction, argIdx);
		enumType = pyStruct[argIdx].enumType;
		
		for (uint32 elemIdx = 0; elemIdx < pyStruct[argIdx].dimensions[0]*pyStruct[argIdx].dimensions[1]; elemIdx++) {

			//Now assign
			
			if (enumType==NPY_UINT8) {
				
				PyMemCopy<uint8>(direction, argIdx, elemIdx, temp);
				
			} else if (enumType==NPY_UINT16) {
				
				PyMemCopy<uint16>(direction, argIdx, elemIdx, temp);
				
			} else if (enumType==NPY_UINT32) {
				
				PyMemCopy<uint32>(direction, argIdx, elemIdx, temp);
				
			} else if (enumType==NPY_UINT64) {
				
				PyMemCopy<uint64>(direction, argIdx, elemIdx, temp);
				
			} else if (enumType==NPY_INT8) {
				
				PyMemCopy<int8>(direction, argIdx, elemIdx, temp);
				
			} else if (enumType==NPY_INT16) {
				
				PyMemCopy<int16>(direction, argIdx, elemIdx, temp);
				
			} else if (enumType==NPY_INT32) {
				
				PyMemCopy<int32>(direction, argIdx, elemIdx, temp);
				
			} else if (enumType==NPY_INT64) {
				
				PyMemCopy<int64>(direction, argIdx, elemIdx, temp);
				
			} else if (enumType==NPY_FLOAT32) {
				
				PyMemCopy<float32>(direction, argIdx, elemIdx, temp);
				
			} else if (enumType==NPY_FLOAT64) {
				
				PyMemCopy<float64>(direction, argIdx, elemIdx, temp);
				
			}  else {
				
				REPORT_ERROR(ErrorManagement::Exception,
							 "Error while refreshing data between model and GAM.");
				
			} 
			
		} 
		
	} 
	
	return ok;
	
} // RefreshData

template <typename T>
void PyGAM::PyMemCopy(const SignalDirection direction, uint32 argIdx, uint32 elemIdx, PyObject* temp) {

	switch (direction) {
		
		case InputSignals:
			
			//*( (T*)PyArray_GETPTR1((PyArrayObject*) temp, elemIdx) ) = *( ( (T*)GetInputSignalMemory(argIdx) ) + elemIdx); // Alternative approach (has some problems with vectors)
			
			*(((T*) PyArray_DATA((PyArrayObject*) temp)) + elemIdx) = *(((T*) GetInputSignalMemory(argIdx)) + elemIdx);
			
			break;
			
		case OutputSignals:
			
			//*(((T*) GetOutputSignalMemory(argIdx)) + elemIdx) = *( (T*)PyArray_GETPTR1((PyArrayObject*) temp, elemIdx) ); // Alternative approach (has some problems with vectors)
			
			*(((T*) GetOutputSignalMemory(argIdx)) + elemIdx) = *(((T*) PyArray_DATA((PyArrayObject*) temp)) + elemIdx);
			
			break;
			
		default:
			
			REPORT_ERROR(ErrorManagement::Exception,
						 "Unsupported data direction.");
			
	}
	
} // PyMemCopy

uint32 PyGAM::NumPyEnumTypeFromMARTe2Type(TypeDescriptor MARTe2Type) {
	
	uint32 enumType;
	
	if (MARTe2Type==UnsignedInteger8Bit) {
		
		enumType = NPY_UINT8;
		
	} else if (MARTe2Type==UnsignedInteger16Bit) {
		
		enumType = NPY_UINT16;
		
	} else if (MARTe2Type==UnsignedInteger32Bit) {
		
		enumType = NPY_UINT32;
		
	} else if (MARTe2Type==UnsignedInteger64Bit) {
		
		enumType = NPY_UINT64;
		
	} else if (MARTe2Type==SignedInteger8Bit) {
		
		enumType = NPY_INT8;
		
	} else if (MARTe2Type==SignedInteger16Bit) {
		
		enumType = NPY_INT16;
		
	} else if (MARTe2Type==SignedInteger32Bit) {
		
		enumType = NPY_INT32;
		
	} else if (MARTe2Type==SignedInteger64Bit) {
		
		enumType = NPY_INT64;
		
	} else if (MARTe2Type==Float32Bit) {
		
		enumType = NPY_FLOAT32;
		
	} else if (MARTe2Type==Float64Bit) {
		
		enumType = NPY_FLOAT64;
		
	}  else {
		
		REPORT_ERROR(ErrorManagement::Exception,
					 "NumPyEnumTypeFromMARTe2Type(): unsupported type.");
		enumType = 0;
		
	}
	
	return enumType;
	
}

CLASS_REGISTER(PyGAM, "1.0")

} // namespace MARTe

/*---------------------------------------------------------------------------*/
/*                           Method definitions                              */
/*---------------------------------------------------------------------------*/

