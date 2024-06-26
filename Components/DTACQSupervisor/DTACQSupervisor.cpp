/**
 * @file EPICSCAClient.cpp
 * @brief Source file for class EPICSCAClient
 * @date 23/03/2017
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
 * the class EPICSCAClient (public, protected, and private). Be aware that some
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
#include "DTACQSupervisor.h"
#include "RegisteredMethodsMessageFilter.h"
#include "ConfigurationDatabase.h"
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
/*---------------------------------------------------------------------------*/
/*                           Static definitions                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                           Method definitions                              */
/*---------------------------------------------------------------------------*/

namespace MARTe {

static FastPollingMutexSem eventCallbackFastMux;
  
  
namespace MARTe {

  
  
  DTACQSupervisor::DTACQSupervisor() :
        ReferenceContainer(), EmbeddedServiceMethodBinderI(), MessageI(){
 
    ReferenceT<RegisteredMethodsMessageFilter> filter = ReferenceT<RegisteredMethodsMessageFilter>(GlobalObjectsDatabase::Instance()->GetStandardHeap());
    filter->SetDestination(this);
    ErrorManagement::ErrorType ret = MessageI::InstallMessageFilter(filter);
    if (!ret.ErrorsCleared()) {
        REPORT_ERROR(ErrorManagement::FatalError, "Failed to install message filters");
    }
}

DTACQSupervisor::~DTACQSupervisor() {

}


bool DTACQSupervisor::Initialise(StructuredDataI & data) {
    bool ok = ReferenceContainer::Initialise(data);
    if (ok) {
        if (!data.Read("CPUs", cpuMask)) {
            REPORT_ERROR(ErrorManagement::Information, "No CPUs defined. Using default = %d", cpuMask);
        }
        if (!data.Read("StackSize", stackSize)) {
            REPORT_ERROR(ErrorManagement::Information, "No StackSize defined. Using default = %d", stackSize);
        }
        if (!data.Read("Name", name)) {
            name = (char *)"MARTE";
        }
        executor.SetStackSize(stackSize);
        executor.SetCPUMask(cpuMask);
        ok = (Start() == ErrorManagement::NoError);
        /*(void) (data.Read("AutoStart", autoStart));
        if (autoStart == 1u) {
            ok = (Start() == ErrorManagement::NoError);
        }*/
    }
    return ok;
}

bool DTACQSupervisor::getGains()
{
    //leggo guadagni
    
}




void DTACQSupervisor::sendMessage(std::string destination, std::string function, std::string argument)
{
    ConfigurationDatabase cdb;
    bool ok = cdb.Write("Class", "Message");
    if (ok) {
	cdb.Write("Destination", destination.c_str());
    }
    if (ok) {
	ok = cdb.Write("Mode", "ExpectsReply");
    }
    if (ok) {
	ok = cdb.Write("Function", function.c_str());
    }
    if (ok) {
	ok = cdb.CreateAbsolute("+Parameters");
    }
    if (ok) {
	ok = cdb.Write("Class", "ConfigurationDatabase");
    }
    if (ok && argument.length() > 0) {
	ok = cdb.Write("param1", argument.c_str());
    }
    if (ok) {
	ok = cdb.MoveToAncestor(1u);
    }
    if (!ok) {
	REPORT_ERROR(ErrorManagement::FatalError, "Could not create ConfigurationDatabase for message");
    }
    if (ok) {
	ReferenceT<Message> message(GlobalObjectsDatabase::Instance()->GetStandardHeap());
	ok = message->Initialise(cdb);
	if (ok) {
	    if (MessageI::SendMessage(message, this) != ErrorManagement::NoError) {
		REPORT_ERROR(ErrorManagement::FatalError, "Could not send message to %s with function  %s  and argumnent %s", destination.c_str(), function.c_str(), argument.c_str());
	    }
	}
	else {
	    REPORT_ERROR(ErrorManagement::FatalError, "Could not Initialise message");
	}
    }
}  
void DTACQSupervisor::sendMessage(std::string destination, std::string function, int32 argument)
{
  
std::cout << "SEND NUMERIC ARG\n";  
  
    ConfigurationDatabase cdb;
    bool ok = cdb.Write("Class", "Message");
    if (ok) {
	cdb.Write("Destination", destination.c_str());
    }
    if (ok) {
	ok = cdb.Write("Mode", "ExpectsReply");
    }
    if (ok) {
	ok = cdb.Write("Function", function.c_str());
    }
    if (ok) {
	ok = cdb.CreateAbsolute("+Parameters");
    }
    if (ok) {
	ok = cdb.Write("Class", "ConfigurationDatabase");
    }
    if (ok) {
	ok = cdb.Write("param1", argument);
    }
    if (ok) {
	ok = cdb.MoveToAncestor(1u);
    }
    if (!ok) {
	REPORT_ERROR(ErrorManagement::FatalError, "Could not create ConfigurationDatabase for message");
    }
    if (ok) {
	ReferenceT<Message> message(GlobalObjectsDatabase::Instance()->GetStandardHeap());
	ok = message->Initialise(cdb);
	if (ok) {
	    if (MessageI::SendMessage(message, this) != ErrorManagement::NoError) {
		REPORT_ERROR(ErrorManagement::FatalError, "Could not send message to %s with function  %s  and argumnent %d", destination.c_str(), function.c_str(), argument);
	    }
	}
	else {
	    REPORT_ERROR(ErrorManagement::FatalError, "Could not Initialise message");
	}
    }
}
ErrorManagement::ErrorType DTACQSupervisor::sendMDSEvent(StreamString name, StreamString value)
{
    std::cout << "SEND MDS EVENT " << name.Buffer() << ", " << value.Buffer() << std::endl;
    MDSplus::Event::setEventRaw((const char *)name.Buffer(), StringHelper::Length(value.Buffer()), (char *)value.Buffer());
    return ErrorManagement::NoError;
}
//setEventRaw(const char *evName, int bufLen, char *buf)
/*
			+ChangeToIdleMsg = {
        		Class = Message
        		Destination = FalconApp
        		Mode = ExpectsReply
        		Function = PrepareNextState
        		+Parameters = {
        			Class = ConfigurationDatabase
        			param1 = Idle
        		}
  



*/



CLASS_REGISTER(DTACQSupervisor, "1.0")
CLASS_METHOD_REGISTER(DTACQSupervisor, Start)
CLASS_METHOD_REGISTER(DTACQSupervisor, sendMDSEvent)
}

 


