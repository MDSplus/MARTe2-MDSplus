
/**
 * @file DTACQAI.cpp
 * @brief Source file for class DTACQAI
 * @date 20 jan 2023
 * @author Gabriele Manduchi
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
 * the class NI6259ADC (public, protected, and private). Be aware that some 
 * methods, such as those inline could be defined on the header file, instead.
 */

/*---------------------------------------------------------------------------*/
/*                         Standard header includes                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                         Project header includes                           */
/*---------------------------------------------------------------------------*/
#include "AdvancedErrorManagement.h"
#include "DTACQAI.h"

#include <stdio.h>
/*---------------------------------------------------------------------------*/
/*                           Static definitions                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                           Method definitions                              */
/*---------------------------------------------------------------------------*/

namespace MARTe {
DTACQAI::DTACQAI() :DataSourceI(), EmbeddedServiceMethodBinderI(), executor(*this) {
    mode = DTACQAI_REALTIME;   //DTACQAI_BULK or DTACQAI_REALTIME
    numAdcs = 0; //Number of ADC boards
    numDis = 0;  //Number of 32 bits digital inputs     
    totAiChans = 0; //Total Number of AI channels
    numAiChans = NULL_PTR(int32);
    calGains = NULL_PTR(float64 *);
    calOffsets = NULL_PTR(float64 *);
    aiChans = NULL_PTR(int16 *);  //Converted AI input channels (BUF_SAMPLE_FACTOR * numSamples * totAiChannels)
    diChans = NULL_PTR(uint32 *);  //DI input channels (BUF_SAMPLE_FACTOR * numSamples)
    times = NULL_PTR(uint32 *); 
    counters = NULL_PTR(uint32 *);  
    chanOffsets = NULL_PTR(uint32 *); //Channel offsets in chanBuf (numSignals)
    chanBuf = NULL_PTR(uint8 *);    //Output buffer
    numSignals = 0; //Number of actually declared signals
    signalIds = NULL_PTR(uint32 *); //For each declared signal the index (i.e. channel id for ai signals(type = float64), bit id for di bits(type = uint8))
    signalTypes = NULL_PTR(uint32 *); //For each declared signal the type (DTACQAI_BULK or DTACKAI_REALTIME)
    numSamples = 0;  //Declared number of output samples
    numBufSamples = 0; //Number of internal (circular) buffer samples (BUF_SAMPLE_FACTOR * numSamples)
    inBufIdx = 0;      //Internal input sample idx (idx of next free sample in the circular buffer)
    outBufIdx = 0;      //Internal Output sample idx (idx of first sample not yet read in the circular buffer)
    port = 0;
    cpuMask = 0;    //CPU Mask for detached thread listening to socket
    packetLen = 0;  //Length of the packet received by D-TACQ system
    packet = NULL_PTR(char8 *);
    if (!synchSem.Create()) {
        REPORT_ERROR(ErrorManagement::FatalError, "Could not create EventSem.");
    }
    bufMux.Create();
} 

DTACQAI::~DTACQAI() 
{
    if (!executor.Stop()) 
    {
        if (!executor.Stop()) 
        {
            REPORT_ERROR(ErrorManagement::FatalError, "Could not stop SingleThreadService.");
        }
    }
    if(aiChans)
    {
        delete [] aiChans;
    }
    if(diChans)
    {
        delete [] diChans;
    }
    if(numAiChans)
    {
        delete [] numAiChans;
    }
    if(calGains)
    {
        delete [] calGains;
    }
    if(calOffsets)
    {
        delete[] calOffsets;
    }
    if(counters)
    {
        delete [] counters;
    }
    if(times)
    {
        delete [] times;
    }
    if(chanOffsets)
    {
        delete[] chanOffsets;
    }
    if(chanBuf)
    {
        delete[] chanBuf;
    }
    if(signalIds)
    {
        delete [] signalIds;
    }
    if(signalTypes)
    {
        delete [] signalTypes;
    }
    if(packet)
    {
        delete[]packet;
    }
}

bool DTACQAI::AllocateMemory() {
    return true;
}

uint32 DTACQAI::GetNumberOfMemoryBuffers() {
    return 1;
}

bool DTACQAI::GetSignalMemoryBuffer(const uint32 signalIdx, const uint32 bufferIdx, void*& signalAddress) {
    bool ok = signalIdx < numSignals;
    if(ok)
    {
        signalAddress = &chanBuf[chanOffsets[signalIdx]];
    }
    return ok;
}


const char8* DTACQAI::GetBrokerName(StructuredDataI& data, const SignalDirection direction) {
    const char8 *brokerName = "MemoryMapSynchronisedInputBroker";
    return brokerName;
}

bool DTACQAI::GetInputBrokers(ReferenceContainer& inputBrokers, const char8* const functionName, void* const gamMemPtr) {
    ReferenceT<MemoryMapSynchronisedInputBroker> broker("MemoryMapSynchronisedInputBroker");
    bool ok = broker.IsValid();
    if (ok) {
        ok = broker->Init(InputSignals, *this, functionName, gamMemPtr);
    }
    if (ok) {
        ok = inputBrokers.Insert(broker);
    }
    return ok;
}
bool DTACQAI::IsSupportedBroker(const SignalDirection direction, const uint32 functionIdx, const uint32 functionSignalIdx, const char8* const brokerClassName)
{
     return true;
}

/*lint -e{715}  [MISRA C++ Rule 0-1-11], [MISRA C++ Rule 0-1-12]. Justification: returns false irrespectively of the input parameters.*/
bool DTACQAI::GetOutputBrokers(ReferenceContainer& outputBrokers, const char8* const functionName, void* const gamMemPtr) {
    return false;
}

/*lint -e{715}  [MISRA C++ Rule 0-1-11], [MISRA C++ Rule 0-1-12]. Justification: the counter and the timer are always reset irrespectively of the states being changed.*/
bool DTACQAI::PrepareNextState(const char8* const currentStateName, const char8* const nextStateName) {
    return true;
}

bool DTACQAI::Synchronise() {
    ErrorManagement::ErrorType err(true);
    uint32 availableSamples;
//    printf("SYNCHRONZE\n");
   // printf("OUT BUF IDX: %d  IN BUF IDX: %d", outBufIdx, inBufIdx);
    bool ok = (bufMux.FastLock() == ErrorManagement::NoError);
    if(!ok)
    {
        REPORT_ERROR(ErrorManagement::FatalError, "Error Acquiring lock");
        return false;
    }
    if (inBufIdx >= outBufIdx) {
        availableSamples = inBufIdx - outBufIdx;
    } 
    else {
        availableSamples = numBufSamples - outBufIdx + inBufIdx;
    } 
    while(availableSamples < numSamples)
    {
        err = !synchSem.Reset(); 
        if(!err.ErrorsCleared())
        {
            REPORT_ERROR(ErrorManagement::FatalError, "Error resetting CSemathore");
            return false;
        }        
        bufMux.FastUnLock();
 //       printf("AVAILABLE SAMPLES: %d\n" ,availableSamples);
        err = synchSem.Wait(TTInfiniteWait);
 //       printf("FINE WAIT\n");
       if(!err.ErrorsCleared())
        {
            REPORT_ERROR(ErrorManagement::FatalError, "Error waiting Condition");
            return false;
        }
        bool ok = (bufMux.FastLock() == ErrorManagement::NoError);
        if(!ok)
        {
            REPORT_ERROR(ErrorManagement::FatalError, "Error Acquiring lock");
            return false;
        }
        if (inBufIdx >= outBufIdx) {
            availableSamples = inBufIdx - outBufIdx;
        } 
        else {
            availableSamples = numBufSamples - outBufIdx + inBufIdx;
        } 
    }
    //At this point there are for sure enough available samples
    uint32 currOffset = 0;
    uint32 currBufIdx = outBufIdx;



    //Copy first counters and times
// printf("WRITING %d SIGNALS %d SAMPLES  AVAILABLE SAMPLES : %d NUM SAMPLES: %d\n", numSignals, numSamples, availableSamples, numSamples);
    for(uint32 currSample = 0; currSample < numSamples; currSample++)
    {
        *((uint32 *)(&chanBuf[currOffset])) = counters[currBufIdx];
        currOffset += sizeof(int32);
        currBufIdx++;
        if(currBufIdx >= numBufSamples)
        {
            currBufIdx = 0;
        }
    }
    currBufIdx = outBufIdx;
    for(uint32 currSample = 0; currSample < numSamples; currSample++)
    {
        *((uint32 *)(&chanBuf[currOffset])) = times[currBufIdx];
        currOffset += sizeof(int32);
        currBufIdx++;
        if(currBufIdx >= numBufSamples)
        {
            currBufIdx = 0;
        }
    }

    for(uint32 currSig = 2; currSig < numSignals; currSig++) 
    {
        currBufIdx = outBufIdx;
        for(uint32 currSample = 0; currSample < numSamples; currSample++)
        {
            if(signalTypes[currSig] == DTACQAI_AI)
            {
                int16 currRaw = aiChans[currBufIdx * totAiChans  + signalIds[currSig]];
                *((float64 *)(&chanBuf[currOffset])) = currRaw * calGains[signalIds[currSig]] + calOffsets[signalIds[currSig]];
                currOffset += sizeof(float64);
            }
            else{
                uint32 currWord = diChans[currBufIdx * numDis + signalIds[currSig]/sizeof(int32)];
                *((uint8 *)(&chanBuf[currOffset])) = ((currWord & (1 << signalIds[currSig%sizeof(uint32)]))>0)?1:0;
                currOffset += sizeof(uint8);
            }
            currBufIdx++;
            if(currBufIdx >= numBufSamples)
            {
                currBufIdx = 0;
            }
        }
    }
    outBufIdx += numSamples;
    if(outBufIdx >= numBufSamples)
    {
        outBufIdx  -= numBufSamples;
    }

    if (inBufIdx >= outBufIdx) {
        availableSamples = inBufIdx - outBufIdx;
    } 
    else {
        availableSamples = numBufSamples - outBufIdx + inBufIdx;
    }
    samplesConsumed = 1;
    bufMux.FastUnLock();
 //   printf("FINITO SYNCH\n");
    return true;
}



bool DTACQAI::Initialise(StructuredDataI& data) {
    bool ok = DataSourceI::Initialise(data);
    if (ok) {
       ok = data.Read("Mode", mode);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "The AI mode (1: Bulk, 2: Realtime) shall be specified");
        }
    }
    if (ok) {
       ok = data.Read("SpadSize", spadSize);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "The SPAD size shall be specified");
        }
    }
    if (ok) {
        ok = data.Read("NumAdc", numAdcs);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "The number of ADC modules shall be specified");
        }
    }
    AnyType arrayDescription;
    if (ok) {
        arrayDescription = data.GetType("NumAiChans");
        ok = arrayDescription.GetDataPointer() != NULL_PTR(void *);
        if(!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "Cannot read NumAiChans");
        }
    }
    if(ok)
    {
        if(arrayDescription.GetNumberOfElements(0u)!= numAdcs) {
                REPORT_ERROR(ErrorManagement::ParametersError, "The number of input channels shall be an array with as many elements as NumAdc");
                ok = false;
        }
    }
    if(ok)
    {
        numAiChans = new int32[numAdcs];
        Vector<int32> readVector(numAiChans, numAdcs);
        ok = data.Read("NumAiChans", readVector);
        if(!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "Error reading NumAiChans");
        }
    }
    totAiChans = 0;
    for(uint32 i = 0; i < numAdcs; i++)
    {
        totAiChans += numAiChans[i]; 
    }
    if (ok) {
       ok = data.Read("NumDi", numDis);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "The number of DIO modules shall be specified");
        }
    }
    if(ok)  {
        arrayDescription = data.GetType("CalGains");
        ok = arrayDescription.GetDataPointer() != NULL_PTR(void *);
        if(!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "Cannot read CalGains");
        }
    }
    if(ok)
    {
        if(arrayDescription.GetNumberOfElements(0u)!= totAiChans) {
                REPORT_ERROR(ErrorManagement::ParametersError, "The number of calibration gains shall be an array with %d elements", totAiChans);
                ok = false;
        }
    }
    if(ok)
    {
        calGains = new float64[totAiChans];
        Vector<float64> readVector(calGains, totAiChans);
        ok = data.Read("CalGains", readVector);
        if(!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "Error reading CalGains");
        }
    }

    if(ok) {
        arrayDescription = data.GetType("CalOffsets");
        ok = arrayDescription.GetDataPointer() != NULL_PTR(void *);
        if(!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "Cannot read CalOffsets");
        }
    }
    if(ok)
    {
        if(arrayDescription.GetNumberOfElements(0u)!= totAiChans) {
                REPORT_ERROR(ErrorManagement::ParametersError, "The number of calibration offsets shall be an array with %d elements", totAiChans);
                ok = false;
        }
    }
    if(ok)
    {
        calOffsets = new float64[totAiChans];
        Vector<float64> readVector(calOffsets, totAiChans);
        ok = data.Read("CalOffsets", readVector);
        if(!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "Error reading CalGains");
        }
    }
    if(ok)
    {
         ok = data.Read("IpAddress", ipAddress);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "ACQ400 IP Address for data readout shall be specified");
        }
    }
    if(ok)
    {
         ok = data.Read("Port", port);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "ACQ400 port for data readout shall be specified");
        }
    }
    if(ok)
    {
        if(!data.Read("CPUs", cpuMask))
        {
            cpuMask = 0;
        }
    }
    if(ok)
    {
         ok = data.Read("TriggerTime", triggerTime);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::Warning, "Trigger time not specified, assumed to be 0");
        }
    }
    if (ok) {
        ok = data.MoveRelative("Signals");
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "Could not move to the Signals section");
        }
    }

    if (ok) {
        numSignals = data.GetNumberOfChildren();
        if(numSignals < 2)
        {
             REPORT_ERROR(ErrorManagement::ParametersError, "DTACQAI shall have at least two signals (Counter, Time)");
             ok = false;
        }
    }
    if(ok)
    {
        if(strcmp(data.GetChildName(0), "Counter") || strcmp(data.GetChildName(1), "Time"))
        {
             REPORT_ERROR(ErrorManagement::ParametersError, "The first two signals of DTACQAI shall shall be Counter and Time");
             ok = false;
        }
    }
    if(ok)
    {
        signalIds = new uint32[numSignals];
        for(uint32 currSig = 2; currSig < numSignals && ok; currSig++)
        {
            data.MoveRelative(data.GetChildName(currSig));
            ok = data.Read("ChannelId", signalIds[currSig]);
            if(!ok) {
                REPORT_ERROR(ErrorManagement::ParametersError, "Channel Id shall be specified for signal %d", currSig);
            }
            data.MoveToAncestor(1u);
        }
        data.MoveToAncestor(1u);
    }
    if(ok)
    {
        if (mode == DTACQAI_REALTIME)
        {
            ok = data.Read("FreqDivision", freqDivision);
            if(!ok) {
                REPORT_ERROR(ErrorManagement::ParametersError, "FreqDuvusion shall be defined for REALTIME mode");
            }
        }
        else
        {
            freqDivision = 1;
        }
    }

    return ok;
}

bool DTACQAI::SetConfiguredDatabase(StructuredDataI& data) {
    uint32 numAiSignals, numDiSignals;
    bool ok = DataSourceI::SetConfiguredDatabase(data);
    if(ok)
    {
        uint32 nOfFunctions = GetNumberOfFunctions();
        ok = (nOfFunctions == 1u);
        if (!ok) {
            REPORT_ERROR(ErrorManagement::ParametersError, "At most one function shall interact with this DataSourceI");
        }
    }
    if (ok) {
        numAiSignals = 0;
        numDiSignals = 0;
        signalTypes = new uint32[numSignals];
        chanOffsets = new uint32[numSignals];
        uint32 currOffset = 0;
        numSamples = 0;
    }
    if(ok)
    {
        uint32 currOffset = 0;
        for (uint32 currSig = 0; currSig < numSignals; currSig++)
        {
            uint32 currSamples = 0u;
            ok = GetFunctionSignalSamples(InputSignals, 0, currSig, currSamples);
            if(!ok)
            {
                REPORT_ERROR(ErrorManagement::ParametersError, "Cannot read number of samples for signal %d", currSig);
                break;
            }
            if(numSamples == 0)
            {
                numSamples = currSamples;
            }
            else
            {
                if(numSamples != currSamples)
                {
                    REPORT_ERROR(ErrorManagement::ParametersError, "All signals shall have the same number %d of samples", numSamples);
                    ok = false;
                    break;
                }
            }
            if(currSig < 2)
            {
                if(GetSignalType(currSig) != SignedInteger32Bit && GetSignalType(currSig) != UnsignedInteger32Bit)  //Counter, Time
                {
                    REPORT_ERROR(ErrorManagement::ParametersError, "Counter and Time shall be of either type int32 or uint32");
                    ok = false;
                    break;
                }
                chanOffsets[currSig] = currOffset;
                currOffset += sizeof(int32) * numSamples;
            }
            else
            {
                if(GetSignalType(currSig) == Float64Bit)
                {
                    numAiSignals++;
                    signalTypes[currSig] = DTACQAI_AI;
                    if(signalIds[currSig] < 0 || signalIds[currSig] >= totAiChans)
                    {
                        REPORT_ERROR(ErrorManagement::ParametersError, "Invalid ChannelId for AI signal %d: Found %d, expected 0-%d", currSig, signalIds[currSig], totAiChans - 1);
                        ok = false;
                        break;
                    }
                    chanOffsets[currSig] = currOffset;
                    currOffset += sizeof(float64)* numSamples;
                }
                else if(GetSignalType(currSig) == UnsignedInteger8Bit || GetSignalType(currSig) == SignedInteger8Bit )
                {
                    numDiSignals++;
                    signalTypes[currSig] = DTACQAI_DI;
                    if(signalIds[currSig] < 0 || signalIds[currSig] >= numDis * 32)
                    {
                        REPORT_ERROR(ErrorManagement::ParametersError, "Invalid ChannelId for DI signal %d: Found %d, expected 0-%d", currSig, signalIds[currSig], numDis * 32 - 1);
                        ok = false;
                        break;
                    }
                    chanOffsets[currSig] = currOffset;
                    currOffset += sizeof(uint8) * numSamples;
                }
                else
                {
                    REPORT_ERROR(ErrorManagement::ParametersError, "Invalid Type for signal %d, it shall be either flot64 or int8 or uint8", currSig);
                    ok = false;
                }
            }
        }
    }
    if(ok)
    {
        chanBuf = new uint8[numSamples*(2*sizeof(int32) + numAiSignals * sizeof(float64) + numDiSignals * sizeof(uint8))];
        aiChans = new int16[BUF_SAMPLE_FACTOR *numSamples * totAiChans];
        diChans = new uint32[BUF_SAMPLE_FACTOR *numSamples * numDis];
        counters = new uint32[BUF_SAMPLE_FACTOR *numSamples];
        times = new uint32[BUF_SAMPLE_FACTOR *numSamples];
        numBufSamples = BUF_SAMPLE_FACTOR * numSamples;
        inBufIdx = 0;      //Internal input sample idx (idx of next free sample in the circular buffer)
        outBufIdx = 0;      //Internal Output sample idx (idx of first sample not yet read in the circular buffer)
        if(mode == DTACQAI_BULK)
        {
            tcpSocket = new TCPSocket();
            if(!tcpSocket->Open())
            {
                 REPORT_ERROR(ErrorManagement::ParametersError, "Cannot Open TCP socket");
                ok = false;
            }
            if(ok)
            {
                if(!tcpSocket->Connect(ipAddress.Buffer(), port))
                {
                    REPORT_ERROR(ErrorManagement::ParametersError, "Cannot Connect TCP socket to %s:%d", ipAddress.Buffer(), port);
                    ok = false;
                }
            }
        }
        else //mode == DTACQAI_REALTIME
        {
            udpSocket = new UDPSocket();
            if(!udpSocket->Open())
            {
                 REPORT_ERROR(ErrorManagement::ParametersError, "Cannot Open UDP socket");
                ok = false;
            }

            if(ok)
            {
               InternetHost ip(port, ipAddress.Buffer());
               printf("UDP: %s\n", ipAddress.Buffer());
                udpSocket->SetSource(ip);
                if(!udpSocket->Listen(port))
                {
                    REPORT_ERROR(ErrorManagement::ParametersError, "Cannot bind to port %d", port);
                    ok = false;
                }
            }
        }
    }
    if(ok)
    {
//        packetLen = numSamples * (totAiChans * sizeof(int16) + numDis * sizeof(uint32) + 64);
//        packetLen = numSamples * (totAiChans * sizeof(int16) + numDis * sizeof(uint32) + 16);
          packetLen = numSamples * (totAiChans * sizeof(int16) + numDis * sizeof(uint32) + spadSize);
//        packetLen = numSamples * (totAiChans * sizeof(int16) + numDis * sizeof(uint32) + 32);

        packet = new char8[packetLen];
    }
    if(ok)
    {
        //Start listening thread
        if (executor.GetStatus() == EmbeddedThreadI::OffState) {
            if (cpuMask != 0u) {
                executor.SetPriorityClass(Threads::RealTimePriorityClass);
                executor.SetCPUMask(cpuMask);
            }
            executor.SetName(GetName());
            ok = executor.Start();
            if(!ok)
            {
                REPORT_ERROR(ErrorManagement::ParametersError, "Cannot start listener thread");
            }
        }
    }
    firstPacket = 1;
    samplesConsumed = 1;
    return ok;
}



ErrorManagement::ErrorType DTACQAI::Execute(ExecutionInfo& info) {
    ErrorManagement::ErrorType err;
    if (info.GetStage() == ExecutionInfo::TerminationStage) {
    }
    else if (info.GetStage() == ExecutionInfo::StartupStage) {
    }
    else {
     //   printf("READING %d BYTES....\n", packetLen);
        uint32 leftBytes = packetLen;
        while(leftBytes > 0)
        {
            uint32 currSize = leftBytes;
            if(mode == DTACQAI_BULK)
            {
              //  printf("LEGGO %d\n", currSize);
                tcpSocket->Read(&packet[packetLen - leftBytes], currSize);
              //  printf("LETTi %d\n", currSize);
            }
            else
            {
                udpSocket->Read(&packet[packetLen - leftBytes], currSize);
            }
            
            leftBytes -= currSize;
        }
//        printf("READ %d\n", inBufIdx);
        bufMux.FastLock();
 //       uint32 singlePacketLen = totAiChans * sizeof(int16) + numDis * sizeof(uint32) + 64;
//        uint32 singlePacketLen = totAiChans * sizeof(int16) + numDis * sizeof(uint32) + 16;
        uint32 singlePacketLen = totAiChans * sizeof(int16) + numDis * sizeof(uint32) + spadSize;
//        uint32 singlePacketLen = totAiChans * sizeof(int16) + numDis * sizeof(uint32) + 32;
        for(uint32 currSample = 0; currSample < numSamples; currSample++)
        {
            counters[inBufIdx] = *((uint32 *)(&packet[currSample * singlePacketLen + totAiChans * sizeof(int16)+numDis*sizeof(uint32)]));
//            printf("\n%d\n", counters[inBufIdx]);
//            uint32 TAITime = *((uint32 *)(&packet[currSample * singlePacketLen + totAiChans * sizeof(int16)+numDis*sizeof(uint32)+2*sizeof(uint32)]));
//            times[inBufIdx] = (TAITime >>28)*1E6 + (TAITime&0x0FFFFFFF)/40;     
            times[inBufIdx] =  *((uint32 *)(&packet[currSample * singlePacketLen + totAiChans * sizeof(int16)+numDis*sizeof(uint32)+sizeof(uint32)]));   
            if(firstPacket)
            {
                firstPacket = 0;
                lastCounter = counters[inBufIdx];
                startTime = times[inBufIdx];
                times[inBufIdx] = 0;
            }
            else
            {
                //if(lastCounter + 1 != (counters[inBufIdx]))
                if(lastCounter + freqDivision != (counters[inBufIdx]))
                {
                    printf("URCA, PACCHETTO PERSO %d  %d\n", lastCounter, counters[inBufIdx]);
                }
                lastCounter = counters[inBufIdx];
                times[inBufIdx] -= startTime;
            }
            for(uint32 i = 0; i < totAiChans; i++)
            {
                aiChans[inBufIdx*totAiChans  + i] = *((int16 *)(&packet[currSample * singlePacketLen +  i * sizeof(int16)]));
     //       printf("%d ", aiChans[inBufIdx + i]);
            }
        //printf("%d  %d  %d\n", counters[inBufIdx], times[inBufIdx], aiChans[inBufIdx*totAiChans]);
          // printf("RAW: %d\n", aiChans[inBufIdx*totAiChans]);
            for(uint32 i = 0; i < numDis; i++)
            {
             diChans[inBufIdx*numDis + i] = *((uint32 *)(&packet[currSample * singlePacketLen + totAiChans * sizeof(int16)+i*sizeof(uint32)]));
            }
            inBufIdx++;
            if(inBufIdx >= numBufSamples)
            {
                inBufIdx = 0;
            }
            if(inBufIdx == outBufIdx)
            {
                printf("OHI OHI: BUFFER OVERFLOW!!!!!!!!!!!\n");
            }
        }
        uint32 availableSamples;
        if (inBufIdx >= outBufIdx) {
            availableSamples = inBufIdx - outBufIdx;
        } 
        else {
            availableSamples = numBufSamples - outBufIdx + inBufIdx;
        }
        if(availableSamples >= numSamples && samplesConsumed)
        {
         //   printf("available samples: %d\n", availableSamples);
            samplesConsumed = 0;
            err = !synchSem.Post();
        }
        bufMux.FastUnLock();
    }
    return err;
}




CLASS_REGISTER(DTACQAI, "1.0")
}


