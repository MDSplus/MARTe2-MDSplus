/**
 * @file ELADSupervisor.cpp
 * @brief Source file for class ELADSupervisor
 * @date 30/8/2023
 * @author Gabriele Manduchi
 *
 */

/*---------------------------------------------------------------------------*/
/*                         Standard header includes                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                         Project header includes                           */
/*---------------------------------------------------------------------------*/

#include "AdvancedErrorManagement.h"
#include "CLASSMETHODREGISTER.h"
#include "ELADSupervisor.h"
#include "RegisteredMethodsMessageFilter.h"
#include "ConfigurationDatabase.h"
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
/*---------------------------------------------------------------------------*/
/*                           Static definitions                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                           Method definitions                              */
/*---------------------------------------------------------------------------*/


//composizione CMD_Register

#define ADG_CH0 0x00
#define ADG_CH1 0x10
#define ADG_CH2 0x20
#define ADG_CH3 0x30
#define MOD_OFF         0x0000
#define MOD_ON          0x8000    //FM si potrà togliere sostituito con EN3V3
#define EN3V3			0x0001
#define SPI_11bit       0x0000
#define SPI_22bit   	0x0080	  //FM changed from 0x0100
#define DMA_STBY        0x00
#define DMA_STOP        0x10
#define DMA_TRG         0x01
#define DMA_ARM         0x02
#define CHOP_EN         0x40
#define tmp_wrADC 1000

#define one_reset   0x800      	//ex 8000 0000
#define one_MR      0x400		//ex 4000 0000
#define one_WR      0x100		//ex 2000 0000
#define one_RD      0x200		//ex 1000 0000
#define one_stby	0x000
#define POLYNOMIAL 0x8C // X^8 + X^5 + X^4 + 1

//#define NUM_ELAD_CHANS 2 






namespace MARTe {

  ELADSupervisor::ELADSupervisor() :
        ReferenceContainer(), EmbeddedServiceMethodBinderI(), MessageI(), executor(*this){
            cpuMask = 1;
            stackSize = THREADS_DEFAULT_STACKSIZE * 4u;
            port = 8000;
            useHwTrig = false;
            useHwAutozeroTrig = false;
            ipReceived = false;
}

ELADSupervisor::~ELADSupervisor() {

}


bool ELADSupervisor::Initialise(StructuredDataI & data) {
    bool ok = ReferenceContainer::Initialise(data);
    if (ok) {
        if (!data.Read("CPUs", cpuMask)) {
            REPORT_ERROR(ErrorManagement::Information, "No CPUs defined. Using default = %d", cpuMask);
        }
        if (!data.Read("StackSize", stackSize)) {
            REPORT_ERROR(ErrorManagement::Information, "No StackSize defined. Using default = %d", stackSize);
        }
        if (!data.Read("Port", port)) {
            REPORT_ERROR(ErrorManagement::Information, "No Port defined. Using default = %d", port);
        }
        printf("APRO DEVICE\n");
        fd = open("/dev/picorfxdriver", O_RDWR | O_SYNC);
        printf("APERTO %d\n", fd);
        if (fd < 0)
        {
            REPORT_ERROR(ErrorManagement::FatalError, "Cannot open ELAD device");
            return false;
        }
//Stop in any case
        uint32 reg =16|MOD_ON; //Stop Streaming
        ioctl(fd, PICORFXDRIVER_SET_COMMAND_REG, &reg);
        usleep(1000);
        reg =0|MOD_ON;
        ioctl(fd, PICORFXDRIVER_SET_COMMAND_REG, &reg);
        usleep(1000);

        executor.SetStackSize(stackSize);
        executor.SetCPUMask(cpuMask);
        serverSock.Open();
        serverSock.Listen(port);
        ok = (executor.Start() == ErrorManagement::NoError);
    }
    return ok;
}


uint32 ELADSupervisor::GetStackSize() const {
    return stackSize;
}

uint32 ELADSupervisor::GetCPUMask() const {
    return cpuMask;
}


ErrorManagement::ErrorType ELADSupervisor::Execute( ExecutionInfo& info) {
    ErrorManagement::ErrorType err = ErrorManagement::NoError;

    if(info.GetStage() == ExecutionInfo::TerminationStage)
        return err;
    else if(info.GetStage() == ExecutionInfo::StartupStage)
    {
        return err;
    }
    printf("WAITING CONNECTION....\n");
    BasicTCPSocket *sock = serverSock.WaitConnection();
    printf("CONNECTED\n");
    while(true)
    {
        printf("Waiting command...\n");
        char8 cmd[4];
        cmd[3] = 0;
        if(!readSock(sock, cmd, 3))
            break;
        if(!handleCommand(sock, cmd))
        {
            break;
        }
    }
    return err;
}


bool ELADSupervisor::readSock(BasicTCPSocket *sock, char *buf, int32 size)
{
    uint32 leftBytes = size;
    uint32 currBytes;
    while(leftBytes > 0)
    {
        currBytes = leftBytes;
        if(!sock->Read(buf + size - leftBytes, currBytes))
            return false;
        leftBytes -= currBytes;
    }
    return true;
}

int ELADSupervisor::shuffle(int data)
{
    char buf[4], tmp;
    int outData;
     if(!endianConversionNeeded)
        return data;
    memcpy(buf, &data, 4);
    tmp = buf[0];
    buf[0] = buf[3];
    buf[3] = tmp;
    tmp = buf[1];
    buf[1] = buf[2];
    buf[2] = tmp;
    memcpy(&outData, buf, 4);
    return outData;

}

bool ELADSupervisor::handleCommand(BasicTCPSocket *sock, char *cmd)
{
    printf("%s\n", cmd);
    uint32 writeBytes = 2;
    uint32 outBytes;
    uint32 intBytes = 4;
    uint32 numSamples;
    uint32 reg;
    uint32 ch = 0;
    uint32 en_chop = 0;
	uint32 cnt_delay=0;

	uint32 max_ch; 
    unsigned int a,i,n,j;  
    unsigned int active_on_ch;
	
    char one_serial2[100];
	char strcrc[50];
    char stringhe_seriali[12][100];
	char stringhe_EE_data[12][100];
	
	int serial_byte=0;
    int one_SN[16];

	uint8_t data[] = {0x12, 0x34, 0x56, 0x78, 0, 0,0};
	
	// common variable setting
    ioctl(fd, PICORFXDRIVER_GET_ADS8900_STAT_REG, &reg);
    max_ch=(reg&0x000000f);
    
	
	
   if(!strcmp(cmd, "CHK"))
    {
        printf("RICEVUTO COMANDO CHK\n");
        int one;
        if(!readSock(sock, (char *)&one, 4))
            return false;
        if(one !=1)
        {
          endianConversionNeeded = true;
          printf("ENDIAN CONVERSION NEEDED\n");
       }
        else
        {
            endianConversionNeeded = false;
            printf("ENDIAN CONVERSION NOT NEEDED\n");
        }

// FM
//            ioctl(fd, PICORFXDRIVER_GET_STATUS_REG, &reg);
//            int32 max_ch=(reg&0x0000f000)>>12;
//        ioctl(fd, PICORFXDRIVER_GET_ADS8900_STAT_REG, &reg);
//        max_ch=(reg&0x000000f);
        printf("ver. 1.00 ADS8900 STATUS =%08x NUMERO CANALI =%d \n",reg,max_ch);

// FM power  only enabled channels 
		reg=EN3V3;	 		  							// 3V3 enabled
	


	a=2;										// and all channels configured in FPGA solo per debug 
		for (j=0; j<max_ch;j++)
		{
		reg=a<<j|reg;	 // M1 to MODn + EN3V3
		ioctl(fd, PICORFXDRIVER_SET_EL_CMD_REG, &reg);	
		usleep(200000);	//aspetta 200ms dopo ogni accensione di modulo new
		}
		printf("ADS8900 PWRW EL_REG =%08x  \n",reg);
		ioctl(fd, PICORFXDRIVER_SET_EL_CMD_REG, &reg);	
		reg =0;
		ioctl(fd, PICORFXDRIVER_GET_EL_CMD_REG, &reg);	
		printf("ADS8900 PWRW EL_REG  =%08x  \n",reg);    //check if correct for future use
		
		//reg =0; sarà utile gestire un reset di tutti gli ADC appena accesi

		reg =7;
		ioctl(fd, PICORFXDRIVER_SET_ADS8900_MODE_REG, &reg);	
		printf("ADS8900 MODE_REG  =%08x  \n",reg);    //check if correct for future use


		
//  il comando qui sotto è lasciato solo per compatibilità DMA  		
            reg =0x8200|0x00;
            reg=MOD_ON|SPI_22bit|ch|DMA_STOP|en_chop;
            ioctl(fd, PICORFXDRIVER_SET_COMMAND_REG, &reg);
            usleep(1000);
  //reg =0x8200|0x10;
            reg=MOD_ON|SPI_22bit|ch|DMA_STBY|en_chop;
            ioctl(fd, PICORFXDRIVER_SET_COMMAND_REG, &reg);
            usleep(1000);

  /////////////////////////////////////////////////////////////////////
  // INIT REGISTRI ADS8900
//old       reg=MOD_ON|SPI_11bit|ch|DMA_STBY|en_chop;                         //--> cmd_reg
			reg=DMA_STBY; 								                       //--> cmd_reg
            ioctl(fd, PICORFXDRIVER_SET_COMMAND_REG, &reg);
            usleep(tmp_wrADC);
			
			reg=SPI_11bit; 								                         //--> ADS8900 cmd_reg
            ioctl(fd, PICORFXDRIVER_SET_ADS8900_CMD_REG, &reg);
            usleep(tmp_wrADC);	
			

			// reg=SPI_22bit;                         //--> cmd_reg
            // ioctl(fd, PICORFXDRIVER_SET_ADS8900_CMD_REG, &reg);
            // usleep(tmp_wrADC);




            reg=0x240c0B00|SPI_22bit;  //wr 11 cicli SPI MODE CONVERSION  -->  ADS8900 cmd_reg   addr 0Ch scrivo SDO_Width 10b SDO_MODE 11b -->11cicli SDO_CNTL Register
            ioctl(fd, PICORFXDRIVER_SET_ADS8900_CMD_REG, &reg);
            usleep(tmp_wrADC);



			ioctl(fd, PICORFXDRIVER_GET_ADS8900_CMD_REG, &reg);	
			printf("ADS8900 CMD  REG =%08x  \n",reg);    //check if correct for future use

            reg=0x240c0300|SPI_22bit;  //wr 22 cicli SPI MODE CONVERSION  --> mod_reg    addr 0Ch scrivo SDO_Width 00b SDO_MODE 11b -->22 cicli SDO_CNTL Register
            ioctl(fd, PICORFXDRIVER_SET_ADS8900_CMD_REG, &reg);
        //usleep(tmp_wrADC);                                                    // test velocità

            reg=0x24100E00|SPI_22bit;  //wr DATA_CTRL PAR_EN=1            --> mod_reg    addr 10h scrivo DATA_CNTL Register no parità
            ioctl(fd, PICORFXDRIVER_SET_ADS8900_CMD_REG, &reg);
        //usleep(tmp_wrADC);


            reg=0x240c0B00|SPI_22bit;  //wr 11 cicli SPI MODE CONVERSION  --> mod_reg   torna nella modalità acquisizione @11 bit
            ioctl(fd, PICORFXDRIVER_SET_ADS8900_CMD_REG, &reg);
            usleep(tmp_wrADC);

			reg=SPI_11bit;                 //--> cmd_reg
            ioctl(fd, PICORFXDRIVER_SET_ADS8900_CMD_REG, &reg);
            usleep(tmp_wrADC);
            printf("\n   \n");
			
			
		// MANCA LA GESTIONE hi/lo + enable chopper	
			
			
			
			
			
  ////////////////////////////////////////////////////////////////////

/*
        //Stop Transient recorder
        reg =4; //Stop Streaming
        ioctl(fd, PICORFXDRIVER_SET_COMMAND_REG, &reg);
        usleep(1000);
        reg =0;
        ioctl(fd, PICORFXDRIVER_SET_COMMAND_REG, &reg);
        usleep(1000);

        reg =16; //Stop Streaming
        ioctl(fd, PICORFXDRIVER_SET_COMMAND_REG, &reg);
        usleep(1000);
        reg =0;
        ioctl(fd, PICORFXDRIVER_SET_COMMAND_REG, &reg);
        usleep(1000);
*/

        if(!sock->Write("OK", writeBytes))
            return false;
        sendFd();
    }
    else if (!strcmp(cmd, "PTS"))
    {
        if(!readSock(sock, (char *)&pts, 4))
            return false;
        pts = shuffle(pts);
        printf("PTS = %d\n", pts);
        ioctl(fd, PICORFXDRIVER_SET_PTS_REG, &pts);
        reg = 0;
        ioctl(fd, PICORFXDRIVER_GET_PTS_REG, &reg);
        printf("Read PTS: %d\n", reg);
        //reg = pts * NUM_ELAD_CHANS * 4;
		reg = pts * max_ch * 4;
        ioctl(fd, PICORFXDRIVER_SET_DMA_BUFLEN, &reg);
        reg = 0;
        ioctl(fd, PICORFXDRIVER_GET_DMA_BUFLEN, &reg);
        printf("Dma Size %d\n", reg);
        if(!sock->Write("OK", writeBytes))
            return false;
    }
    else if(!strcmp(cmd, "MOD"))
    {
        uint32 modeReg;
        if(!readSock(sock, (char *)&modeReg, 4))
            return false;
        modeReg = shuffle(modeReg);
		modeReg =modeReg|0x00040000; 
        printf("ADS8900_MODE_REG: forza ch 3 LH1\n");
		printf("ADS8900_MODE_REG: %x\n", modeReg);
        ioctl(fd, PICORFXDRIVER_SET_ADS8900_MODE_REG, &modeReg);
		
        if(!sock->Write("OK", writeBytes))
            return false;
    }
    else if (!strcmp(cmd, "STA"))
    {
        uint32 statReg = 0;
//        ioctl(fd, PICORFXDRIVER_GET_ADS8900_STAT_REG, &statReg);
//Pero ora ci metto io a mano i valori a regime sara' chiamata la ioctl sopra 
		//statReg = 2;
		statReg = max_ch;
		
        uint32 statBytes = 4;
        if(!sock->Write((char *)&statReg, statBytes))
            return false;
    }
    else if (!strcmp(cmd, "IDS"))
    {
// Per ora dummy, Franco lo riempi tu con le opportune chiamate al driver
		printf("ID Modules start-------------------------\n");
 

// ------------------leggi tutti i seriali ------readIDs--------------------------------

printf("legge tutti i seriali disponibili-------------------------------------\n");
printf("max_ch  %d \n",max_ch);

for (n=0; n<12;n++) {
		
		for (j=0;j<32;j++){
		stringhe_seriali[n][j]= '0';
		stringhe_EE_data[n][j]= '0';
		
		}		
		stringhe_seriali[n][16]='\0';
		stringhe_EE_data[n][32]='\0';
		
}



 // for (n=0; n<12;n++) {
		 // printf("zz %s %s \n",stringhe_seriali[n],stringhe_EE_data[n]);	
 // }
// inizio loop

for (active_on_ch=0; active_on_ch<12;active_on_ch++) {         //leggi tutti gli IDs indifferentemente dal numero di canali compilati in FPGA (max_ch)
															   // indifferentemente anche se non alimentati, le memorie vengono viste lo stesso
															   // questo permette di sapere quali moduli sono montati sulla mother board		
	//for (active_on_ch=0; active_on_ch<max_ch;active_on_ch++) {
	//printf(" comando RESET BUS on channel %d\n",active_on_ch);
	reg=one_MR|(active_on_ch<<12);	 		  				// activate Master Reset
 	 ioctl(fd, PICORFXDRIVER_SET_ONE_CMD_REG, &reg);
	 for(j=1; j<8; j++);									// small delay
	 reg=one_stby|(active_on_ch<<12);	 					// deactivate Master Reset
     ioctl(fd, PICORFXDRIVER_SET_ONE_CMD_REG, &reg);
	 cnt_delay=one_wait_busy_low(fd);
	 //printf("delay one wire busy su reset %d \n",cnt_delay);  
 	 //printf("delay one wire busy su cmd33 %d \n",one_wait_busy_low(fd));	 
	 // comando Write 33 BUS 
	 reg=0x33|one_WR|(active_on_ch<<12);	 		  // setta comando Write 33 BUS 
 	 ioctl(fd, PICORFXDRIVER_SET_ONE_CMD_REG, &reg);
	 reg=0x33|(active_on_ch<<12);	 		  		  // resetta
     ioctl(fd, PICORFXDRIVER_SET_ONE_CMD_REG, &reg);
 	 cnt_delay=	one_wait_busy_low(fd);
// 	 printf("delay one wire busy su cmd33 %d \n",cnt_delay);
// 	 printf("delay one wire busy su cmd33 %d \n",one_wait_busy_low(fd));	 
	cnt_delay++; //fittizio tanto perchè il compilatore non mi dia errore se non usata la variabile	
	 // comando Read SN BUS 
	for (i=0; i<8; i++)
		{
		// comando Read BUS 
		reg=one_RD|(active_on_ch<<12);	 		  //
		ioctl(fd, PICORFXDRIVER_SET_ONE_CMD_REG, &reg);  
		reg=one_stby|(active_on_ch<<12);	 		  //--> cmd_reg
		ioctl(fd, PICORFXDRIVER_SET_ONE_CMD_REG, &reg);
	    cnt_delay=	one_wait_busy_low(fd);
	//	printf("delay one wire busy su bit %d %d \n",i, cnt_delay);
	//		printf("delay one wire busy su bit %d %d \n",i, one_wait_busy_low(fd));

			 ioctl(fd, PICORFXDRIVER_GET_ONE_STAT_REG, &reg);
		one_SN[i]=reg&0xFF;
		}
sprintf (one_serial2,"%02X%02X%02X%02X%02X%02X%02X%02X",one_SN[7],one_SN[6],one_SN[5],one_SN[4],one_SN[3],one_SN[2],one_SN[1],one_SN[0]);

 
 for (i=0; i<sizeof(data); i++)
{
	data[i]=uint8_t(one_SN[i]);
}
 uint8_t crc = computeMaximCRC8(data, sizeof(data));
 if (crc==uint8_t(one_SN[7]))
{
sprintf(strcrc,"CRC OK");
strncpy(stringhe_seriali[active_on_ch],one_serial2,20); //write ID only if CRC is correct 
}
	else{
	sprintf(strcrc,"CRC KO NOOOOOOO");
}
 
//strncpy(stringhe_seriali[active_on_ch],one_serial2,20);
printf ("> %s >> %s >>> CH%2d %s \n",one_serial2,stringhe_seriali[active_on_ch],active_on_ch+1,strcrc );

	 // comando Write F0 BUS 
	 reg=0xF0|one_WR|(active_on_ch<<12);	 		  // setta comando Write 33 BUS 
 	 ioctl(fd, PICORFXDRIVER_SET_ONE_CMD_REG, &reg);
	 reg=0xF0|(active_on_ch<<12);	 		  		  // resetta
     ioctl(fd, PICORFXDRIVER_SET_ONE_CMD_REG, &reg);
	 	 cnt_delay=one_wait_busy_low(fd);
 	 //printf("delay one wire busy su cmdF0 %d \n",one_wait_busy_low(fd));

	 // comando Write 00 indirizzo pagina0 BUS 
	 reg=0x00|one_WR|(active_on_ch<<12);	 		  // setta comando Write 33 BUS 
 	 ioctl(fd, PICORFXDRIVER_SET_ONE_CMD_REG, &reg);
	 reg=0x00|(active_on_ch<<12);	 		  		  // resetta
     ioctl(fd, PICORFXDRIVER_SET_ONE_CMD_REG, &reg);
 	 	 cnt_delay=one_wait_busy_low(fd);
	 //printf("delay one wire busy su add 00 %d \n",one_wait_busy_low(fd));

	 reg=0x00|one_WR|(active_on_ch<<12);	 		  // setta comando Write 33 BUS 
 	 ioctl(fd, PICORFXDRIVER_SET_ONE_CMD_REG, &reg);
	 reg=0x00|(active_on_ch<<12);	 		  		  // resetta
     ioctl(fd, PICORFXDRIVER_SET_ONE_CMD_REG, &reg);
 	 	 cnt_delay=one_wait_busy_low(fd);
	 //printf("delay one wire busy su add 00 %d \n",one_wait_busy_low(fd));

	 // comando Read SN BUS 
	for (i=0; i<16; i++)
		{
		// comando Read BUS 
		reg=one_RD|(active_on_ch<<12);	 		  //
		ioctl(fd, PICORFXDRIVER_SET_ONE_CMD_REG, &reg);  
		reg=one_stby|(active_on_ch<<12);	 		  //--> cmd_reg
		ioctl(fd, PICORFXDRIVER_SET_ONE_CMD_REG, &reg);
			 cnt_delay=one_wait_busy_low(fd);
		//printf("delay one wire busy su bit %d %d \n",i, one_wait_busy_low(fd));
			 ioctl(fd, PICORFXDRIVER_GET_ONE_STAT_REG, &reg);
		one_SN[i]=reg&0xFF;
		}
sprintf (one_serial2,"%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",one_SN[0],one_SN[1],one_SN[2],one_SN[3],one_SN[4],one_SN[5],one_SN[6],one_SN[7],one_SN[8],one_SN[9],one_SN[10],one_SN[11],one_SN[12],one_SN[13],one_SN[14],one_SN[15]);

strncpy(stringhe_EE_data[active_on_ch],one_serial2,32);


	 reg=one_MR|(active_on_ch<<12);	 		  				// disable MEMORY activating Master Reset
 	 ioctl(fd, PICORFXDRIVER_SET_ONE_CMD_REG, &reg);
	 for(j=1; j<8; j++);									// small delay
	 reg=one_stby|(active_on_ch<<12);	 					// deactivate Master Reset
     ioctl(fd, PICORFXDRIVER_SET_ONE_CMD_REG, &reg);
	 cnt_delay=one_wait_busy_low(fd);
	 //printf("delay one wire busy su reset %d \n",one_wait_busy_low(fd));  
}  // fine loop

for (n=0; n<12;n++) {
		printf("ID %2d %s   EE pag0 memo->  %s \n",n,stringhe_seriali[n],stringhe_EE_data[n]);
}
// ------------------ fine ------readIDs--------------------------------

 for(int i = 0; i < 12; i++)
        {
            uint32 idBytes = 16;
            if(!sock->Write(stringhe_seriali[i], idBytes))
	        return false;
        }
    }
    else if (!strcmp(cmd, "DIV"))
    {
        if(!readSock(sock, (char *)&clockDiv, 4))
            return false;
        clockDiv = shuffle(clockDiv);
        printf("CLOCK DIV = %d\n", clockDiv);
        ioctl(fd, PICORFXDRIVER_SET_FREQ_DIV_REG, &clockDiv);
        reg = 0;
        ioctl(fd, PICORFXDRIVER_GET_FREQ_DIV_REG, &reg);
        printf("Read CLOCK DIV: %d\n", reg);
        if(!sock->Write("OK", writeBytes))
            return false;
    }
    else if (!strcmp(cmd, "AUT"))
    {
        uint32 autozeroTicks;
        if(!readSock(sock, (char *)&autozeroTicks, 4))
            return false;
        autozeroTicks = shuffle(autozeroTicks);
        autozeroDuration = autozeroTicks*1E-3;
        printf("AUTOZERO DURATION = %f\n", autozeroDuration);
        reg = autozeroDuration*1000000;
        ioctl(fd, PICORFXDRIVER_SET_AUTOZERO_SAMPLES_REG, &reg);
        usleep(10);
        ioctl(fd, PICORFXDRIVER_GET_AUTOZERO_SAMPLES_REG, &reg);
        printf("Autozero samples: %d\n", reg);

        double div = 1./(autozeroDuration*1000000);
        div *= pow(2,40);
        reg = (int)(div + 0.5);
        ioctl(fd, PICORFXDRIVER_SET_AUTOZERO_MUL_REG, &reg);
        usleep(10);
        ioctl(fd, PICORFXDRIVER_GET_AUTOZERO_MUL_REG, &reg);
        printf("Autozero mul factor: %d\n", reg);
        usleep(10);
        if(!sock->Write("OK", writeBytes))
            return false;
    }
    else if (!strcmp(cmd, "TEX"))
    {
        int32 ext;
        if(!readSock(sock, (char *)&ext, 4))
            return false;
        ext = shuffle(ext);
        printf("EXT = %d\n", ext);
        useHwTrig = (ext == 1);
        if(!sock->Write("OK", writeBytes))
            return false;
    }
    else if (!strcmp(cmd, "TAU"))
    {
        int32 ext;
        if(!readSock(sock, (char *)&ext, 4))
            return false;
        ext = shuffle(ext);
        printf("TAU = %d\n", ext);
        useHwAutozeroTrig = (ext == 1);
        if(!sock->Write("OK", writeBytes))
            return false;
    }
    else if (!strcmp(cmd, "IPP"))
    {
        uint32 ipLen;
        if(!readSock(sock, (char *)&ipLen, 4))
            return false;
        printf("LEN: %d\n", ipLen);
        char8 *ipStr = new char8[ipLen+1];
        ipStr[ipLen] = 0;
        if(!readSock(sock, (char *)ipStr, ipLen))
            return false;
        printf("LETTO %s", ipStr);
        if(!readSock(sock, (char *)&udpOutPort, 4))
            return false;
        if(!ipReceived)
        {
            ipReceived = true;
            udpOutIp.Write(ipStr, ipLen);
            printf("PORT: %d\n", udpOutPort);
            sendIp();
        }
        if(!sock->Write("OK", writeBytes))
            return false;
    }
    else if (!strcmp(cmd, "ARM"))
    {
        ioctl(fd, PICORFXDRIVER_ARM_DMA, 0);
        printf("Dma Armed\n");



        printf("Riepilogo registri -------------->\n");
        ioctl(fd, PICORFXDRIVER_GET_ADS8900_STAT_REG, &reg);
        printf("ADS900 STAT REG %X\n",reg );
        ioctl(fd, PICORFXDRIVER_GET_ADS8900_MODE_REG, &reg);
        printf("ADS900 MODE REG %X\n",reg );
		ioctl(fd, PICORFXDRIVER_GET_ADS8900_CMD_REG, &reg);
        printf("ADS900 CMD REG %X\n",reg );
		ioctl(fd, PICORFXDRIVER_GET_EL_CMD_REG, &reg);
        printf("EL CMD REG %X\n",reg );		
		ioctl(fd, PICORFXDRIVER_GET_EL_STAT_REG, &reg);
        printf("EL STAT REG %X\n",reg );			


		
		
        reg=1;
        usleep(10);
        ioctl(fd, PICORFXDRIVER_START_DMA, &reg);
        printf("Dma Started\n");
        usleep(10);


//FM            reg=MOD_ON|SPI_11bit|ch|DMA_ARM|en_chop;

            reg=DMA_ARM;
        ioctl(fd, PICORFXDRIVER_SET_COMMAND_REG, &reg);
 //       reg = 2;
 //       ioctl(fd, PICORFXDRIVER_SET_COMMAND_REG, &reg);
        printf("ADC  Armed \n");

        if(!sock->Write("OK", writeBytes))
            return false;
    }
    else if (!strcmp(cmd, "TRG"))
    {
 //FM		reg = MOD_ON|1;
 		reg = 1;
        ioctl(fd, PICORFXDRIVER_SET_COMMAND_REG, &reg);
        printf("ADC  triggered\n");
        usleep(10);
         if(!sock->Write("OK", writeBytes))
            return false;
    }
    else if (!strcmp(cmd, "TRS"))
    {
  //FM     reg = MOD_ON|(1<<10);
		reg = (1<<10);
 		ioctl(fd, PICORFXDRIVER_SET_COMMAND_REG, &reg);
        printf("Autozero  triggered\n");
        usleep(10);
  //     reg = MOD_ON;
  		reg = 0x00;
        ioctl(fd, PICORFXDRIVER_SET_COMMAND_REG, &reg);
         if(!sock->Write("OK", writeBytes))
            return false;
    }
////////////////////Lettura conttenuto EPROM
    else if (!strcmp(cmd, "EPR"))
    {
     /*  
			for(uint32 i = 0; i < 12; i++)
        {
            //PER FRANCO, mettere qui la lettura dell'eprom per ogni canale
            //uint32 numChanBytes = strlen("CICCIOBOMBO");
            //char *chanBuf = "CICCIOBOMBO";
			uint32 numChanBytes = strlen(stringhe_EE_data);
            if(!sock->Write(&numChanBytes, sizeof(uint32)))
                return false;
//            if(!sock->Write(chanBuf, numChanBytes))
			 if(!sock->Write(&stringhe_EE_data, numChanBytes))
			return false;
        }*/
			for(int i = 0; i < 12; i++)
        {
            uint32 idBytes = 32;
	        if(!sock->Write(stringhe_EE_data[i], idBytes))
	        return false;
        }
		
		
    }
    else if (!strcmp(cmd, "CMR"))
    {
        uint32 8900CmdReg;
        if(!readSock(sock, (char *)&8900CmdReg, 4))
            return false;
        ioctl(fd, PICORFXDRIVER_SET_ADS8900_CMD_REG, &8900CmdReg);
    }
    else if (!strcmp(cmd, "MDR"))
    {
        uint32 8900ModeReg;
        if(!readSock(sock, (char *)&8900ModeReg, 4))
            return false;
        ioctl(fd, PICORFXDRIVER_SET_ADS8900_MODE_REG, &8900CmdReg);
    }
    else if (!strcmp(cmd, "ECR"))
    {
        uint32 elCmdReg;
        if(!readSock(sock, (char *)&elCmdReg, 4))
            return false;
        ioctl(fd, PICORFXDRIVER_SET_EL_CMD_REG, &8900CmdReg);
    }


/////////////////////////////////////////////////

    else if (!strcmp(cmd, "STS"))
    {
        reg = 0;
        ioctl(fd, PICORFXDRIVER_GET_STREAM_FIFO_LEN, &reg);
        printf("FIFO LEN: %d\n", reg);
        for(uint32 i = 0; i < reg; i++)
            ioctl(fd, PICORFXDRIVER_GET_STREAM_FIFO_VAL, &reg);
        ioctl(fd, PICORFXDRIVER_CLEAR_STREAM_FIFO, 0);
        usleep(10);

        ioctl(fd, PICORFXDRIVER_START_READ, 0);
        enableStream(true);
 //FM   reg = 8|MOD_ON;
		reg = 8;
 		ioctl(fd, PICORFXDRIVER_SET_COMMAND_REG, &reg);
        printf("Streaming started\n");
        usleep(10);
 //FM   reg = 0|MOD_ON;
		reg = 0;
 		ioctl(fd, PICORFXDRIVER_SET_COMMAND_REG, &reg);
         if(!sock->Write("OK", writeBytes))
            return false;

    }
    else if (!strcmp(cmd, "STO"))
    {
 //FM       reg = (1<<9)|MOD_ON;
        reg = (1<<9);
        ioctl(fd, PICORFXDRIVER_SET_COMMAND_REG, &reg);
        usleep(10);
 //FM       reg = 0|MOD_ON;
        reg = 0;
		ioctl(fd, PICORFXDRIVER_SET_COMMAND_REG, &reg);
        enableStream(false);
        printf("Streaming stopped\n");
        if(!sock->Write("OK", writeBytes))
            return false;
    }
    else if (!strcmp(cmd, "STR"))
    {
        //numSamples = pts*NUM_ELAD_CHANS;
		numSamples = pts*max_ch;
		//int32 outSamples = pts*NUM_ELAD_CHANS;
        int32 outSamples = pts*max_ch;
        int32 *samples = new int32[numSamples];
        int32 *chanSamples = new int32[numSamples];
        ioctl(fd, PICORFXDRIVER_STOP_DMA, 0);
        printf("Dma Stopped\n");
        ioctl(fd, PICORFXDRIVER_GET_DMA_DATA, samples);
        printf("Got DMA data\n");

 //FM       uint32 off=0x8000;
 //       reg=off|SPI_11bit|ch|DMA_STBY;                    //--> cmd_reg
        uint32 off=0x8000;
        reg=ch|DMA_STBY;                    //--> cmd_reg
		ioctl(fd, PICORFXDRIVER_SET_COMMAND_REG, &reg);
        usleep(1000);



        if(endianConversionNeeded)
        {
            //for(int32 i = 0; i < NUM_ELAD_CHANS*pts; i++)
            for(uint32 i = 0; i < max_ch*pts; i++)
            {
                samples[i] = shuffle(samples[i]);
            }
            outSamples = shuffle(outSamples);
        }
        uint32 outIdx = 0;
        //for(int32 chan = 0; chan < NUM_ELAD_CHANS; chan++)
		for(uint32 chan = 0; chan < max_ch; chan++)
		{
            for(int32 i = 0; i < pts; i++)
            {
                //chanSamples[outIdx++] = samples[i*NUM_ELAD_CHANS+chan];
                chanSamples[outIdx++] = samples[i*max_ch+chan];
            }
        }
        if(!sock->Write((char8 *)&outSamples, intBytes))
            return false;
        outBytes = numSamples * sizeof(int32);


        if(!sock->Write((char8 *)chanSamples, outBytes))
        {
            printf("ERRORE IN SCRITTURA SOCKET\n");
            return false;
        }
        delete [] samples;
        delete [] chanSamples;
    }
    return  true;
}

void ELADSupervisor::sendFd()
{
       ConfigurationDatabase cdb;
        bool ok = cdb.Write("Class", "Message");
        if (ok) {
                cdb.Write("Destination", "ELADRT.Data.ELAD");
        }
       if (ok) {
                ok = cdb.Write("Mode", "\"ExpectIndirectsReply\"");
        }
        if (ok) {
                ok = cdb.Write("Function", "setFd");
        }
        if (ok) {
                ok = cdb.CreateAbsolute("+Parameters");
        }
        if (ok) {
                ok = cdb.Write("Class", "ConfigurationDatabase");
        }
        if (ok) {
                ok = cdb.Write("param1", fd);
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
                        REPORT_ERROR(ErrorManagement::FatalError, "Could not send message to ELAD with function  setFd  and argumnent %d", fd);
                }
            }
            else {
                REPORT_ERROR(ErrorManagement::FatalError, "Could not Initialise message");
            }
    }
}
void ELADSupervisor::enableStream(bool enable)
{
       ConfigurationDatabase cdb;
        bool ok = cdb.Write("Class", "Message");
        if (ok) {
                cdb.Write("Destination", "ELADRT.Data.ELAD");
        }
       if (ok) {
                ok = cdb.Write("Mode", "\"ExpectIndirectsReply\"");
        }
        if (ok) {
                ok = cdb.Write("Function", "enableStream");
        }
        if (ok) {
                ok = cdb.CreateAbsolute("+Parameters");
        }
        if (ok) {
                ok = cdb.Write("Class", "ConfigurationDatabase");
        }
        if (ok) {
                ok = cdb.Write("param1", enable?1:0);
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
                        REPORT_ERROR(ErrorManagement::FatalError, "Could not send message to ELAD with function  setFd  and argumnent %d", fd);
                }
            }
            else {
                REPORT_ERROR(ErrorManagement::FatalError, "Could not Initialise message");
            }
    }
}

void ELADSupervisor::sendIp()
{
       ConfigurationDatabase cdb;
        bool ok = cdb.Write("Class", "Message");
        if (ok) {
                cdb.Write("Destination", "ELADRT.Data.UDP_OUT");
        }
       if (ok) {
                ok = cdb.Write("Mode", "\"ExpectIndirectsReply\"");
        }
        if (ok) {
                ok = cdb.Write("Function", "setIp");
        }
        if (ok) {
                ok = cdb.CreateAbsolute("+Parameters");
        }
        if (ok) {
                ok = cdb.Write("Class", "ConfigurationDatabase");
        }
        printf("SCRIVO 1 %d\n", ok);
        if (ok) {
                ok = cdb.Write("param1", udpOutIp);
        }
       printf("SCRIVO 2 %d\n", ok);
        if (ok) {
                ok = cdb.Write("param2", udpOutPort);
        }
        printf("SCRIVO 3 %d\n", ok);
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
                        REPORT_ERROR(ErrorManagement::FatalError, "Could not send message to ELAD with function  setFd  and argumnent %d", fd);
                }
            }
            else {
                REPORT_ERROR(ErrorManagement::FatalError, "Could not Initialise message");
            }
    }
}

uint8_t ELADSupervisor::computeMaximCRC8(const uint8_t* data, size_t length) {
    uint8_t crc = 0x00;
    
    for (size_t byte = 0; byte < length; byte++) {
        crc ^= data[byte];
        
        for (int bit = 0; bit < 8; bit++) {
            if (crc & 0x01) {
                crc = (crc >> 1) ^ POLYNOMIAL;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}



int ELADSupervisor::one_wait_busy_low(int fd){
	int	j=0;
	unsigned int regtemp;
for (j=0; j<400; j++)   //wait for busy with timeout
	{
	ioctl(fd, PICORFXDRIVER_GET_ONE_STAT_REG, &regtemp);
	if ((regtemp&0x80000000)==0) break;
	 }
	 return j;
} 


CLASS_REGISTER(ELADSupervisor, "1.0")
}


