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
#include "MDSEventManager.h"
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
  
  class MarteEvent:public MDSplus::Event
  {
    MDSEventManager *evManager;
    public:
      MarteEvent(MDSEventManager *evManager):Event(evManager->name.Buffer())
      {
	this->evManager = evManager;
      }
      void run()
      {
        const char *name = getName(); //Get the name of the event
        char *date = getTime()->getDate(); //Get the event reception date in string format
	size_t bufSize, pos1, pos2;
	const char *buf =  getRaw(&bufSize); //Get raw data
        char *str = new char[bufSize+1]; //Make it a string
        memcpy(str, buf, bufSize);
        str[bufSize] = 0;
        std::cout << "RECEIVED EVENT " << name << " AT " << date << " WITH DATA  " << str << "\n";
	std::string inStr(str);
	if (inStr == "EXIT")
	{
	  std::cout << "EXITING MARTe" << std::endl;
	  kill(getpid(),SIGTERM); 
          sleep(2);
          exit(0);
	}
	pos1 = inStr.find(":");
	bool ok = (pos1 != std::string::npos);
	if(ok)
	{
	  std::string destination = inStr.substr(0,pos1);
	  std::string function;
	  pos2 = inStr.find(":", pos1+1);
	  if(pos2 != std::string::npos)
	  {
	    function = inStr.substr(pos1+1, pos2 - pos1 - 1);
	    std::string argument = inStr.substr(pos2+1);
	    std::cout << "Destination: " << destination << "   Function: " << function << "  Argument:  " << argument << std::endl;
/*	    if(argument[0] >='0' && argument[0] <= '9')
		evManager->sendMessage(destination, function, atoi(argument.c_str()));
	    else
*/		evManager->sendMessage(destination, function, argument);
	  }
	  else
	  {
	    function = inStr.substr(pos1+1);
	    std::cout << "Destination: " << destination << "   Function: " << function << std::endl;
	    evManager->sendMessage(destination, function, "");
	    
	  }
	    
	}
	else
	  std::cout << "ERROR: Could not handle MDS message "<< str << std::endl;
      }
  };

} 



  
namespace MARTe {

  
  
  MDSEventManager::MDSEventManager() :
        ReferenceContainer(), EmbeddedServiceMethodBinderI(), MessageI(), executor(*this){
    stackSize = THREADS_DEFAULT_STACKSIZE * 4u;
    cpuMask = 0xffu;
    eventCallbackFastMux.Create();
    ReferenceT<RegisteredMethodsMessageFilter> filter = ReferenceT<RegisteredMethodsMessageFilter>(GlobalObjectsDatabase::Instance()->GetStandardHeap());
    filter->SetDestination(this);
    ErrorManagement::ErrorType ret = MessageI::InstallMessageFilter(filter);
    if (!ret.ErrorsCleared()) {
        REPORT_ERROR(ErrorManagement::FatalError, "Failed to install message filters");
    }
 //   signal(SIGTERM, StopApp);
 //   signal(SIGINT, StopApp);

}

MDSEventManager::~MDSEventManager() {

}


bool MDSEventManager::Initialise(StructuredDataI & data) {
    printf("\n\n\nMDS EVENTS INIT\n\n\n");
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

ErrorManagement::ErrorType MDSEventManager::Start() {
    ErrorManagement::ErrorType err = executor.Start();
    return err;
}

EmbeddedThreadI::States MDSEventManager::GetStatus() {
    return executor.GetStatus();
}


ErrorManagement::ErrorType MDSEventManager::Execute( ExecutionInfo& info) {
    ErrorManagement::ErrorType err = ErrorManagement::NoError;
    if (info.GetStage() == ExecutionInfo::StartupStage) {
        (void) eventCallbackFastMux.FastLock();
 	eventManager = new MarteEvent(this);
	eventManager->start();
        REPORT_ERROR(ErrorManagement::Information, "MDS Event Listener started");
        eventCallbackFastMux.FastUnLock();
    }
    else if (info.GetStage() != ExecutionInfo::BadTerminationStage) {
        Sleep::Sec(1.0);
    }
    else {
        (void) eventCallbackFastMux.FastLock();
	delete eventManager;
        eventCallbackFastMux.FastUnLock();
    }
	
    return err;
}

uint32 MDSEventManager::GetStackSize() const {
    return stackSize;
}

uint32 MDSEventManager::GetCPUMask() const {
    return cpuMask;
}


void MDSEventManager::sendMessage(std::string destination, std::string function, std::string argument)
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
void MDSEventManager::sendMessage(std::string destination, std::string function, int32 argument)
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
ErrorManagement::ErrorType MDSEventManager::sendMDSEvent(StreamString name, StreamString value)
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



CLASS_REGISTER(MDSEventManager, "1.0")
CLASS_METHOD_REGISTER(MDSEventManager, Start)
CLASS_METHOD_REGISTER(MDSEventManager, sendMDSEvent)
}

 


