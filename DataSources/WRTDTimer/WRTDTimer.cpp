/**
 * @file WRTDTimer.cpp
 * @brief Source file for class WRTDTimer
 * @date 25/10/2016
 * @author Gabriele Manduchi
 *
 */

#define DLL_API

/*---------------------------------------------------------------------------*/
/*                         Standard header includes                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                         Project header includes                           */
/*---------------------------------------------------------------------------*/
#include "AdvancedErrorManagement.h"
#include "MemoryMapSynchronisedInputBroker.h"
#include "ObjectRegistryDatabase.h"
#include "ReferenceContainerFilterReferences.h"
#include "WRTDTimer.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <regex.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <math.h>
#include <errno.h>


/*---------------------------------------------------------------------------*/
/*                           Static definitions                              */
/*---------------------------------------------------------------------------*/
namespace MARTe {
/**
 * @brief Execute in the context of the real-time thread.
 */

/**
 * @brief Maximum phase of the signal (default)
 */
const uint32 USEC_IN_SEC = 1000000u;
}

/*---------------------------------------------------------------------------*/
/*                           Method definitions                              */
/*---------------------------------------------------------------------------*/
namespace MARTe {

#define CLOCKFD 3
static clockid_t get_clockid(int fd)
{
  return (((unsigned int) ~fd) << 3) | CLOCKFD;
}

static void getTime(const void * const parameter) 
{
    WRTDTimer *timer = (WRTDTimer *)(parameter);
    timer->handleTimeMessage();
}



WRTDTimer::WRTDTimer() :
        DataSourceI(),
        EmbeddedServiceMethodBinderI(),
        executor(*this) {
    
        phase = 0;
        period = 0;
        delay = 0;
        leapSeconds = 0;
        messageReceived = false;
        counter = 0u;
        * (reinterpret_cast<int64 *>(time))= 0;
        trigger = 0;
        absoluteTime = 0;
        if (!synchSem.Create()) {
            REPORT_ERROR(ErrorManagement::FatalError, "Could not create EventSem.");
        }
}

WRTDTimer::~WRTDTimer() {
    if (!synchSem.Post()) {
        REPORT_ERROR(ErrorManagement::FatalError, "Could not post EventSem.");
    }
    if (!executor.Stop()) {
        if (!executor.Stop()) {
            REPORT_ERROR(ErrorManagement::FatalError, "Could not stop SingleThreadService.");
        }
    }
}

bool WRTDTimer::AllocateMemory() {
    return true;
}

bool WRTDTimer::Initialise(StructuredDataI &data) {
    bool ok = DataSourceI::Initialise(data);
    ConfigurationDatabase slaveCDB;

    if (ok) {
        if (!data.Read("ClockName", clockName)) {
            REPORT_ERROR(ErrorManagement::ParametersError, "ClockName shall be defined");
            ok = false;
        }
        if ((clockName !=  "CLOCK_TAI") && (clockName != "CLOCK_REALTIME") &&(clockName != "CLOCK_MONOTONIC")&&(clockName != "CLOCK_PTP"))
        {
           REPORT_ERROR(ErrorManagement::ParametersError, "Invalid Clock mode");
            ok = false;
        }
    }
    if(ok) {

        if (!data.Read("Phase", phase)) {
            phase = 0u;
            REPORT_ERROR(ErrorManagement::Information, "Phase was not configured, using default %d", phase);
        }
        else {
            REPORT_ERROR(ErrorManagement::Information, "Phase manually configured to %d", phase);
        }
        if (!data.Read("Delay", delay)) {
            delay = 0u;
            REPORT_ERROR(ErrorManagement::Information, "Delay was not configured, using default %d", delay);
        }
        else {
            REPORT_ERROR(ErrorManagement::Information, "Delay manually configured to %d", delay);
        }
    }
    if(ok) {       
        if (!data.Read("Period", period)) {
            REPORT_ERROR(ErrorManagement::Information, "Period shall be specified");
            ok = false;
        }
    }
    if(ok) {       
        if (!data.Read("LeapSeconds", leapSeconds)) {
            REPORT_ERROR(ErrorManagement::Information, "LeapSeconds shall be specified");
            ok = false;
        }
    }
    if(ok) {
        if (!data.Read("EventName", eventName)) {
            REPORT_ERROR(ErrorManagement::Information, "EventName shall be specified");
            ok = false;
        }
    }
    if(ok) {
        if (!data.Read("MulticastGroup", multicastGroup)) {
            REPORT_ERROR(ErrorManagement::Information, "MulticastGroup shall be specified");
            ok = false;
        }
    }
    if(ok) {
        if (!data.Read("UdpPort", udpPort)) {
            REPORT_ERROR(ErrorManagement::ParametersError, "UdpPort shall be specified");
            ok = false;
        }
    }
    if (ok) {
        uint32 cpuMaskIn;
        if (!data.Read("CpuMask", cpuMaskIn)) {
            cpuMaskIn = 0xFFu;
            REPORT_ERROR(ErrorManagement::Warning, "CPUMask not specified using: %d", cpuMaskIn);
        }
        cpuMask = ProcessorType(cpuMaskIn);


        if (ok) {
            executor.SetCPUMask(cpuMask);
            executor.SetStackSize(THREADS_DEFAULT_STACKSIZE);
        }
    }
    return ok;
}

bool WRTDTimer::SetConfiguredDatabase(StructuredDataI &data) {
    bool ok = DataSourceI::SetConfiguredDatabase(data);
    uint32 tempNumOfSignals = GetNumberOfSignals();

    if (ok) {
        ok = (tempNumOfSignals >= 3u) && (tempNumOfSignals <= 4u);
    }
    if (!ok) {
        REPORT_ERROR(ErrorManagement::ParametersError, "Number of signal must be between 3 and 4");
    }
    if (ok) {
        ok = (GetSignalType(0u).numberOfBits == 32u);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "The first signal shall have 32 bits and %d were specified", uint16(GetSignalType(0u).numberOfBits));
        }
    }
    if (ok) {
        ok = (GetSignalType(0u).type == SignedInteger);
        if (!ok) {
            ok = (GetSignalType(0u).type == UnsignedInteger);
        }
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "The first signal (Counter) shall SignedInteger or UnsignedInteger type");
        }
    }
    if (ok) {
        ok = (GetSignalType(1u).numberOfBits == 64 || GetSignalType(1u).numberOfBits == 32);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "The second signal (Time) shall either 64 or 32 bits and %d ",
                         uint16(GetSignalType(1u).numberOfBits));
        }
        time32Bits = (GetSignalType(1u).numberOfBits == 32);
    }
    if (ok) {
        ok = (GetSignalType(1u).type == SignedInteger);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "The second signal (Time) shall SignedInteger type");
        }
    }
    if (ok) {
        ok = (GetSignalType(2u).numberOfBits == 8);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "The third signal (Trigger) shall be either int8 or uint8");
        }
    }

    if (tempNumOfSignals > 3u) {
        uint16 tempNumOfBits = GetSignalType(3u).numberOfBits;

        ok = ((GetSignalType(3u).type == UnsignedInteger) && (tempNumOfBits == 64u));
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "The third signal must be a 64 bit unsigned integer");
        }
    }
    return ok;
}

uint32 WRTDTimer::GetNumberOfMemoryBuffers() {
    return 1u;
}


bool WRTDTimer::GetSignalMemoryBuffer(const uint32 signalIdx,
                                       const uint32 bufferIdx,
                                       void *&signalAddress) {
    bool ok = true;
    if (signalIdx == 0u) {
        signalAddress = &counter;
    }
    else if (signalIdx == 1u) {
        signalAddress = &time;
    }
    else if (signalIdx == 2u) {
        signalAddress = &trigger;
    }
    else if (signalIdx == 3u) {
        signalAddress = &absoluteTime;
    }
    else {
        ok = false;
    }
    return ok;
}

const char8* WRTDTimer::GetBrokerName(StructuredDataI &data,
                                       const SignalDirection direction) {

    const char8 *brokerName = NULL_PTR(const char8*);

    if (direction == InputSignals) {
            brokerName = "MemoryMapSynchronisedInputBroker";
    }
    else {
        REPORT_ERROR(ErrorManagement::ParametersError, "DataSource not compatible with OutputSignals");
    }
    return brokerName;
}

bool WRTDTimer::Synchronise() {
    ErrorManagement::ErrorType err;
    err = synchSem.ResetWait(TTInfiniteWait);
    printf("WRTD Time: %ld\n", *(long int *)time);
    return err.ErrorsCleared();
}

bool WRTDTimer::PrepareNextState(const char8 *const currentStateName,
                                  const char8 *const nextStateName) {
    bool ok = true;
    stopped = true; //Force stooping clock generation 
    printf("PREPARE NEXT STATE\n");
    if (executor.GetStatus() == EmbeddedThreadI::OffState) {
        ok = executor.Start();
    }
    return ok;
}

ErrorManagement::ErrorType WRTDTimer::Execute(ExecutionInfo &info) {

    ErrorManagement::ErrorType err;
    struct timespec currTp;
    uint32 status;
    float64 currStartTime;
    uint32 baseCounter;
    uint32 currCounter = 0;
    messageReceived = false;
    stopped = false;
    Threads::BeginThread(&getTime, this, THREADS_DEFAULT_STACKSIZE, "TimeReceiverThread", ExceptionHandler::NotHandled, cpuMask);



    uint32 clockId = wrtdGetClockId(clockName.Buffer());
    if ((status = clock_gettime(clockId, &currTp)) != 0)
    {
        return ErrorManagement::FatalError;
    }
    currStartTime = currTp.tv_sec + currTp.tv_nsec*1.E-9;
    stopped = false;

    while(!stopped)
    {
        if(messageReceived && (currStartTime + (currCounter+2) * period > startTime + delay)) //Keep initial clock active until close to real start
        if(messageReceived && (currStartTime + (currCounter+2) * period > startTime + delay)) //Keep initial clock active until close to real start
            break;
        currCounter++;
        wrtdWaitUntil(clockId, currStartTime + currCounter * period, 0);  
        float64 currTime =  (delay - period) * 1E6;
        if(time32Bits) 
        {
            * (reinterpret_cast<int32 *>(time)) = currTime;
        }
        else
        {
            * (reinterpret_cast<int64 *>(time)) = currTime;
        }
        
        trigger = 0;
        counter = 0;
        absoluteTime =  (currStartTime + currCounter * period)*1E6; 
#ifdef DEBUG
        REPORT_ERROR(ErrorManagement::Debug, "Time %d", time);    
        REPORT_ERROR(ErrorManagement::Debug, "Trigger %d", trigger);    
        REPORT_ERROR(ErrorManagement::Debug, "AbsoluteTime %d", absoluteTime);  
#endif  
        synchSem.Post();
    }
    if(stopped)
    {
        return ErrorManagement::NoError;
    }
    baseCounter = currCounter;
    //startTime has already been set
    while(!stopped)
    {
     //   REPORT_ERROR(ErrorManagement::Debug, "Curr time %f period: %f counter: %d", currStartTime, period, currCounter); 
     //   REPORT_ERROR(ErrorManagement::Debug, "Wait until %f", startTime + + delay + phase + (currCounter - baseCounter) * period); 
        wrtdWaitUntil(clockId, startTime + delay + phase + (currCounter - baseCounter) * period, 0);
        float64 currTime = (delay + phase + (currCounter - baseCounter) * period) * 1E6;
        if(time32Bits) 
        {
            * (reinterpret_cast<int32 *>(time)) = currTime;
        }
        else
        {
            * (reinterpret_cast<int64 *>(time)) = currTime;
        }
        trigger = 1;
        absoluteTime = (startTime + delay + phase + (currCounter - baseCounter) * period) * 1E6;
        currCounter++;
        counter = currCounter;
#ifdef DEBUG
        REPORT_ERROR(ErrorManagement::Debug, "Time %d", time); 
        REPORT_ERROR(ErrorManagement::Debug, "Trigger %d", trigger);    
        REPORT_ERROR(ErrorManagement::Debug, "AbsoluteTime %d", absoluteTime);    
#endif
        synchSem.Post();
    }
    return ErrorManagement::NoError;
}



const ProcessorType& WRTDTimer::GetCPUMask() const {
    return cpuMask;
}

uint32 WRTDTimer::GetStackSize() const {
    return THREADS_DEFAULT_STACKSIZE;
}





uint32 WRTDTimer::wrtdGetClockId(const char *clock_name)
{
    unsigned int clkid;
    struct timespec cur_time;
    if (strcmp (clock_name, "CLOCK_TAI") == 0)
        clkid = CLOCK_TAI;
    else if (strcmp (clock_name, "CLOCK_REALTIME") == 0)
       clkid = CLOCK_REALTIME;
    else if (strcmp (clock_name, "CLOCK_MONOTONIC") == 0)
        clkid = CLOCK_MONOTONIC;
    else if (strlen(clock_name) == 0)
	clkid = 0;
    else
    {
        const char *ptp_front = "/dev/ptp";
        if (strncmp(clock_name, ptp_front, strlen(ptp_front)) == 0)
        {
                int fd = open(clock_name, O_RDWR);
                if (fd < 0)
            {
                perror("could not open PtP clock");
                return fd;
            }
            clkid = get_clockid(fd); 	
        }
        else
        {
            REPORT_ERROR(ErrorManagement::ParametersError,"Unrecognized clock '%s' must be one of ('', 'CLOCK_REALTIME', 'CLOCK_TAI', 'CLOCK_MONOTONIC', '/dev/ptpN')\n", clock_name);
            return -1;
        }
    
    }
    return clkid;
}


float64 WRTDTimer::wrtdGetTime(const char *group, unsigned int port, const char *event_regex, unsigned int clock_id, double delay, int leapseconds, unsigned int verbose)
{
    double answer = -1.;
    regex_t reegex;
    unsigned int ptp = ((clock_id != CLOCK_TAI) &&
                      (clock_id != CLOCK_REALTIME) &&
                      (clock_id != CLOCK_MONOTONIC));

    if (verbose)
    {
	    printf("wrtdGetTime(%s, %u, %s, %d, %f, %d)\n", group, port, event_regex, clock_id, delay, leapseconds);
    }
  // Compile the regular expression
  if (regcomp( &reegex, event_regex, 0)) {
    perror("Could not parse regex");
    return answer;
  }

  // create what looks like an ordinary UDP socket
  //
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    perror("socket");
    return answer;
  }

  // allow multiple sockets to use the same PORT number
  //
  u_int yes = 1;
  if (setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, (char*) &yes, sizeof(yes)) < 0)
  {
   perror("Reusing ADDR failed");
   return answer;
  }

  // set up destination address
  //
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY); // differs from sender
  addr.sin_port = htons(port);

  // bind to receive address
  //
  if (bind(fd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
    perror("bind");
    return answer;
  }

  // use setsockopt() to request that the kernel join a multicast group
  //
  struct ip_mreq mreq;
  mreq.imr_multiaddr.s_addr = inet_addr(group);
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);
  if ( setsockopt( fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq, sizeof(mreq) ) < 0 )
  {
    perror("setsockopt");
    return answer;
  }
  int status = 1;
  while (1)
  {
    struct wrtd_message msgbuf;
    socklen_t addrlen = sizeof(addr);
    int nbytes = recvfrom( fd, (char *)&msgbuf, sizeof(msgbuf), 0, (struct sockaddr *) &addr, &addrlen );
    if (nbytes < 0) {
        perror("recvfrom");
        break;
    }
    if (nbytes != sizeof(msgbuf))
    {
        if (verbose)
            REPORT_ERROR(ErrorManagement::Information,"recfrom expected %d bytes got %d\n", (int)sizeof(msgbuf), nbytes);
        continue;
    }


    if (strncmp((const char *)msgbuf.hw_detect, "LXI", 3))
    {
        if (verbose)
            REPORT_ERROR(ErrorManagement::Information,"Expected LXI message got -%3.3s-\n", msgbuf.hw_detect);
        continue;
    }
    if (regexec( &reegex, (const char *)msgbuf.event_id, 0, NULL, 0))
    {
        if (verbose)
            REPORT_ERROR(ErrorManagement::Information,"LXI message -%s- is not for us -%s-\n", msgbuf.event_id, event_regex);
        continue;
    }
    if (verbose)
        REPORT_ERROR(ErrorManagement::Information,"hw_detect -%3.3s-\nevent_id -%s-\nseq %d\nts_sec %d\nts_ns %d\nts_frac %d\nts_hi_sec %d\n",
              (char *)msgbuf.hw_detect,(char *)msgbuf.event_id, msgbuf.seq, msgbuf.ts_sec, msgbuf.ts_ns, msgbuf.ts_frac, msgbuf.ts_hi_sec);
    double delay_secs = floor(delay);
 //   answer = msgbuf.ts_sec - leapseconds + (int)delay_secs + msgbuf.ts_ns*1E-9 + (delay - delay_secs)*1E9; 
 //Mi pare un fia' sballata
    //answer = msgbuf.ts_sec - leapseconds + (int)delay_secs + msgbuf.ts_ns*1E-9 + delay; GAB NATALE
    answer = msgbuf.ts_sec - leapseconds + msgbuf.ts_ns*1E-9 + delay;
    break;
  }
  if (verbose)
  {
        REPORT_ERROR(ErrorManagement::Information,"wrtdGetTime returning %f\n", answer);
  }
  return answer;
} 


int32 WRTDTimer::wrtdWaitUntil(uint32 clock_id, float64 until, uint32 verbose)
{
    int status;
    struct timespec desired_tp, cur_tp;
    desired_tp.tv_sec = floor(until);
    desired_tp.tv_nsec = (until - desired_tp.tv_sec) * 1E9;
    if ((status = clock_gettime(clock_id, &cur_tp)) == 0)
    {
        struct timespec remaining_tp={0,0};
        if (verbose)
        {
            printf("CURRENT: tv_sec=%ld tv_nsec=%ld\n", cur_tp.tv_sec, cur_tp.tv_nsec);
            printf("DESIRED: tv_sec=%ld tv_nsec=%ld\n", desired_tp.tv_sec, desired_tp.tv_nsec);
            printf("sleeping\n");
        }
        if (
          (clock_id == CLOCK_TAI) ||
          (clock_id == CLOCK_REALTIME) ||
          (clock_id == CLOCK_MONOTONIC)
         )
        {
            while((status = clock_nanosleep(clock_id, TIMER_ABSTIME, &desired_tp, &remaining_tp)))
            {
                if (status != EINTR)
                {
                    perror("nanosleep - unexpected return value");
                    return status;
                }
            }
            if (verbose)
            {
                printf("tv_sec=%ld tv_nsec=%ld\n", desired_tp.tv_sec, desired_tp.tv_nsec);
                printf("\tawake status = %d\n", status);
            }
        }
        else
        {
            if (verbose)
                printf("PTP Sleeping\n");
            while((cur_tp.tv_sec < desired_tp.tv_sec) || (cur_tp.tv_nsec < desired_tp.tv_nsec))
            {    
                if ((status = clock_gettime(clock_id, &cur_tp)))
                {      
                    perror("Failed to get PTP time\n");
                    return status;
                }      
            }
	        if (verbose)
	        {
	            printf("PtP Awake tv_sec = %ld, tv_nsec = %ld\n", cur_tp.tv_sec, cur_tp.tv_nsec);
	        }
        }
    }
    else
    {
      perror("could not clock_gettime");
    }
    return status;
}

void WRTDTimer::handleTimeMessage()
{
    float64 answer;
    uint32 clockId = wrtdGetClockId(clockName.Buffer());
   // answer = wrtdGetTime(multicastGroup.Buffer(),  udpPort, eventName.Buffer(), clockId, delay, leapSeconds, 1);
    answer = wrtdGetTime(multicastGroup.Buffer(),  udpPort, eventName.Buffer(), clockId, 0, leapSeconds, 1);
    startTime = answer;
    messageReceived = true;
}

CLASS_REGISTER(WRTDTimer, "1.0")

}
