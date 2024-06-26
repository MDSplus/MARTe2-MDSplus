#include <stdio.h>
#include "acq400_chapi.h"
#include <string.h>
#include <string>
#include <iostream>
//#include <bits/stdc++.h>

int uutGet(acq400_chapi::Acq400 uut, std::string &rxMessage, int site, const char *command)
{
    char siteStr[16];
    sprintf(siteStr, "%d", site);
    int rc = uut.get(&rxMessage, siteStr, command);
    if (rc > 0){
        std::cout << "result " << rxMessage << std::endl;
    }else{
        fprintf(stderr, "ERROR: sr returned %d\n", rc);
    }
    return rc;
}
  
int uutSet(acq400_chapi::Acq400 &uut, int site, const char *key, const char *val)
{
    char siteStr[16];
    sprintf(siteStr, "%d", site);
    int rc = uut.set(siteStr, key, val);
    return rc;
}
/*
 * DTACQ configuration: 
 * extClock: 1:external, 0: internal
 * clockFreq: clock frequency
 * dioSite: site of DIO. -1 means no DIO
 * aiSites: sites of AI modules
 * numAiChans: number of AI channels per site
 * numAiSites: number of AI sites
 * dioDirs: array of 4 integers:dioDir[i] = 0 means that the corresponding byte is output(input)
 * aiOffsets: returned array of AI offsets (allocated by the caller)
 * aiGains: returned array of AI gains (allocated by the caller)
 */

int configure(char *host, int extClock, int clockFreq, int dioSite, int *aiSites, int *numAiChans, int numAiSites, 
              double *aiGains, double *aiOffsets, int spadSize)
{
    char buf[256];
    acq400_chapi::Acq400 uut(host);
    std::cout << "Connected" << std::endl;
    if(extClock)
    {
        sprintf(buf, "fpmaster %d %d", clockFreq, clockFreq);
    }
    else
    {
        sprintf(buf, "master %d", clockFreq);
    }
    uutSet(uut, 0, "sync_role", buf);
    uutSet(uut, 0, "transient", "SOFT_TRIGGER=0");
    uutSet(uut, 0, "spad1_us", "1,1,1");
    uutSet(uut, 0, "spad1_us_clk_src", "0");
    if(dioSite > 0)
    {
        uutSet(uut, dioSite, "byte_is_output", "0,0,0,0");
    }
    int offsetIdx = 0, gainIdx = 0;
    for (int aiIdx = 0; aiIdx < numAiSites; aiIdx++)
    {
        std::string retOffsetsStr;
        uutGet(uut, retOffsetsStr, aiSites[aiIdx], "AI:CAL:EOFF");
       
        char *tempBuf = new char[retOffsetsStr.size()+1];
        strcpy(tempBuf, retOffsetsStr.c_str());
        char *token;
        std::cout << tempBuf << std::endl;
        token = strtok(tempBuf, " ");
        token = strtok(NULL, " ");
        token = strtok(NULL, " ");
        while(true)
        {
            token = strtok(NULL, " ");
            if(!token)
                break;
            sscanf(token, "%lf", &aiOffsets[offsetIdx++]);
        }
        delete [] tempBuf;
        
        std::string retGainsStr;
        uutGet(uut, retGainsStr, aiSites[aiIdx], "AI:CAL:ESLO");
        tempBuf = new char[retOffsetsStr.size()+1];
        strcpy(tempBuf, retGainsStr.c_str());
        token = strtok(tempBuf, " ");
        token = strtok(NULL, " ");
        token = strtok(NULL, " ");
        while(true)
        {
            token = strtok(NULL, " ");
            if(!token)
                break;
            sscanf(token, "%lf", &aiGains[gainIdx++]);
        }
        delete [] tempBuf;
    }
    printf("%d gains and %d offsets\n", gainIdx, offsetIdx);
    char stBuf[256];
    stBuf[0] = 0;
    for (int aiIdx = 0; aiIdx < numAiSites; aiIdx++)
    {
        sprintf(&stBuf[strlen(stBuf)], "%d,", aiSites[aiIdx]);
    }
    if(dioSite > 0)
    {
        sprintf(&stBuf[strlen(stBuf)], "%d,", dioSite);
    }
    sprintf(&stBuf[strlen(stBuf)], " 1,%d,0", spadSize);
    uutSet(uut, 0, "run0", stBuf);
}
    
    
    
    
    
    

int main(int argc, char **argv) {
    if (argc < 2){
        fprintf(stderr, "USAGE: acq400_chapi_acq400_test UUT \n");
        exit(1);
    }
    char* host = argv[1];
    std::cout << "Host: " << argv[1] << std::endl;
    int aiSites[] = {1,2};
    int numAiChans[]={32,32};
    double aiGains[64], aiOffsets[64];
    configure(host, 1, 10000, 6, aiSites, numAiChans, 2, aiGains, aiOffsets, 16);
    for(int i = 0; i < 64; i++)
    {
        printf("%lf\t%lf\n", aiGains[i], aiOffsets[i]);
    }
  
}
