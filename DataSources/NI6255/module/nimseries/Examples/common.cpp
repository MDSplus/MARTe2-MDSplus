//
//  common.cpp
//
//  $DateTime: 2006/10/24 23:40:45 $
//
#include "common.h"

void configureTimebase (tMSeries* board)
{
    board->Clock_and_FOUT.setSlow_Internal_Timebase(1);
    board->Clock_and_FOUT.flush();
    
    return;
}

void pllReset (tMSeries* board)
{
    board->Clock_And_Fout2.setTB1_Select(tMSeries::tClock_And_Fout2::kTB1_SelectSelect_OSC);
    board->Clock_And_Fout2.setTB3_Select(tMSeries::tClock_And_Fout2::kTB3_SelectSelect_OSC);
    board->Clock_And_Fout2.flush();
    
    board->PLL_Control.setPLL_Enable (kFalse);
    board->PLL_Control.flush ();
    
    return;
}

void analogTriggerReset (tMSeries* board)
{
    board->Analog_Trigger_Etc.setAnalog_Trigger_Reset(1);
    board->Analog_Trigger_Etc.setAnalog_Trigger_Mode(tMSeries::tAnalog_Trigger_Etc::kAnalog_Trigger_ModeLow_Window);
    board->Analog_Trigger_Etc.flush();
    
    board->Analog_Trigger_Control.setAnalog_Trigger_Select(tMSeries::tAnalog_Trigger_Control::kAnalog_Trigger_SelectGround);
    board->Analog_Trigger_Control.flush();
    
    board->Gen_PWM[0].writeRegister(0);
    board->Gen_PWM[1].writeRegister(0);
    
    board->Analog_Trigger_Etc.setAnalog_Trigger_Enable(tMSeries::tAnalog_Trigger_Etc::kAnalog_Trigger_EnableDisabled);
    board->Analog_Trigger_Etc.flush();
    
    return; 
}

typedef enum 
{
   kPFI_Default_Out   = 0,
   kAI_Start1_PFI     = 1,
   kAI_Start2_PFI     = 2,
   kAI_Convert        = 3,
   kG1_Selected_SRC   = 4,
   kG1_Selected_GATE  = 5,
   kAO_UPDATE_N       = 6,
   kAO_Start1_PFI     = 7,
   kAI_Start_Pulse    = 8,
   kG0_Selected_SRC   = 9,
   kG0_Selected_GATE  = 10,
   kExtStrobe         = 11,
   kAI_ExternalMUX_Clk    = 12,
   kG0_Out            = 13,
   kG1_Out            = 14,
   kFreq_Out          = 15,
   kPFI_DO            = 16,
   kI_Atrig           = 17,
   kRTSI_Pin0         = 18,
   kRTSI_Pin1         = 19,
   kRTSI_Pin2         = 20,
   kRTSI_Pin3         = 21,
   kRTSI_Pin4         = 22,
   kRTSI_Pin5         = 23,
   kRTSI_Pin6         = 24,
   kRTSI_Pin7         = 25,
   kStar_Trig_In      = 26,
   kSCXI_Trig1        = 27,
   kDIO_ChangeDetect_RTSI       = 28,
   kCDI_Sample        = 29,
   kCDO_Update        = 30,
} tPFI_Output_Select;

typedef enum {
   kADR_START1        = 0,
   kADR_START2        = 1,
   kSCLKG             = 2,
   kDACUPDN           = 3,
   kDA_START1         = 4,
   kG_SRC_0           = 5,
   kG_GATE_0          = 6,
   kRGOUT0            = 7,
   kRTSI_BRD_0        = 8,
   kRTSI_BRD_1        = 9,
   kRTSI_BRD_2        = 10,
   kRTSI_BRD_3        = 11,
   kRTSI_OSC          = 12,
} tRTSI_Output_Select;

typedef enum {
   kMUX_Out_Sel_PFI0  = 0,
   kMUX_Out_Sel_PFI1  = 1,
   kMUX_Out_Sel_PFI2  = 2,
   kMUX_Out_Sel_PFI3  = 3,
   kMUX_Out_Sel_PFI4  = 4,
   kMUX_Out_Sel_PFI5  = 5,
   kMUX_Out_Sel_AO_Gate_RTSI          = 6,
   kMUX_Out_Sel_AI_Gate_RTSI          = 7,
   kMUX_Out_Sel_Freq_Out      = 8,
   kMUX_Out_Sel_G1_Out    = 9,
   kMUX_Out_Sel_G1_Selected_Gate              = 10,
   kMUX_Out_Sel_G1_Selected_Source                = 11,
   kMUX_Out_Sel_G1_Z  = 12,
   kMUX_Out_Sel_G0_Z  = 13,
   kMUX_Out_Sel_Analog_Trigger            = 14,
   kMUX_Out_Sel_AI_Start_Pulse            = 15,
} tRTSI_Shared_MUX_Output_Select;
      
void exportSignal (tMSeries* board, 
                    tSignal source, 
                    tTerminal terminal,
                    tBoolean enable)
{
 
   tPFI_Output_Select             PFI_Output_Select = kPFI_Default_Out;
   tRTSI_Output_Select            RTSI_Output_Select;
   tRTSI_Shared_MUX_Output_Select RTSI_Shared_MUX_Output_Select = kMUX_Out_Sel_PFI0;
   
   tBoolean isRTSITerminal = (terminal < kPFITerminal);
   
   //
   // Configure RTSI and PFI according to the source signal
   //
   switch (source)
   {
      case kAISampleClock:
         PFI_Output_Select = kAI_Start_Pulse;
         RTSI_Output_Select = kRTSI_BRD_0;
         RTSI_Shared_MUX_Output_Select = kMUX_Out_Sel_AI_Start_Pulse; 
         break;
      
      case kAIConvertClock:
         PFI_Output_Select = kAI_Convert;
         RTSI_Output_Select = kSCLKG;
         break;
      
      case kAIStartTrigger:
         PFI_Output_Select = kAI_Start1_PFI;
         RTSI_Output_Select = kADR_START1;
         break;
      
      case kAIReferenceTrigger:
         PFI_Output_Select = kAI_Start2_PFI;
         RTSI_Output_Select = kADR_START2;
         break;
      
      case kAOSampleClock:
         PFI_Output_Select = kAO_UPDATE_N;
         RTSI_Output_Select = kDACUPDN;
         break;
      
      case kAOStartTrigger:
         PFI_Output_Select = kAO_Start1_PFI;
         RTSI_Output_Select = kDA_START1;
         break;
      
      default:
         return;
   }
   
   //
   // Commit to hardware
   //
   switch (terminal)
   {
      //
      // Set RTSI terminal
      //
      case kRTSI0:
         board->RTSI_Trig_A_Output.writeRTSI0_Output_Select((tMSeries::tRTSI_Trig_A_Output::tRTSI0_Output_Select)RTSI_Output_Select);
         board->RTSI_Trig_Direction.writeRTSI0_Pin_Dir((tMSeries::tRTSI_Trig_Direction::tRTSI0_Pin_Dir)enable);
         break;
      
      case kRTSI1:
         board->RTSI_Trig_A_Output.writeRTSI1_Output_Select((tMSeries::tRTSI_Trig_A_Output::tRTSI1_Output_Select)RTSI_Output_Select);
         board->RTSI_Trig_Direction.writeRTSI1_Pin_Dir((tMSeries::tRTSI_Trig_Direction::tRTSI1_Pin_Dir)enable);
         break;
      
      case kRTSI2:
         board->RTSI_Trig_A_Output.writeRTSI2_Output_Select((tMSeries::tRTSI_Trig_A_Output::tRTSI2_Output_Select)RTSI_Output_Select);
         board->RTSI_Trig_Direction.writeRTSI2_Pin_Dir((tMSeries::tRTSI_Trig_Direction::tRTSI2_Pin_Dir)enable);
         break;
      
      case kRTSI3:
         board->RTSI_Trig_A_Output.writeRTSI3_Output_Select((tMSeries::tRTSI_Trig_A_Output::tRTSI3_Output_Select)RTSI_Output_Select);
         board->RTSI_Trig_Direction.writeRTSI3_Pin_Dir((tMSeries::tRTSI_Trig_Direction::tRTSI3_Pin_Dir)enable);
         break;
      
      case kRTSI4:
         board->RTSI_Trig_B_Output.writeRTSI4_Output_Select((tMSeries::tRTSI_Trig_B_Output::tRTSI4_Output_Select)RTSI_Output_Select);
         board->RTSI_Trig_Direction.writeRTSI4_Pin_Dir((tMSeries::tRTSI_Trig_Direction::tRTSI4_Pin_Dir)enable);
         break;
      
      case kRTSI5:
         board->RTSI_Trig_B_Output.writeRTSI5_Output_Select((tMSeries::tRTSI_Trig_B_Output::tRTSI5_Output_Select)RTSI_Output_Select);
         board->RTSI_Trig_Direction.writeRTSI5_Pin_Dir((tMSeries::tRTSI_Trig_Direction::tRTSI5_Pin_Dir)enable);
         break;
      
      case kRTSI6:
         board->RTSI_Trig_B_Output.writeRTSI6_Output_Select((tMSeries::tRTSI_Trig_B_Output::tRTSI6_Output_Select)RTSI_Output_Select);
         board->RTSI_Trig_Direction.writeRTSI6_Pin_Dir((tMSeries::tRTSI_Trig_Direction::tRTSI6_Pin_Dir)enable);
         break;
      
      case kRTSI7:
         board->RTSI_Trig_B_Output.writeRTSI7_Output_Select((tMSeries::tRTSI_Trig_B_Output::tRTSI7_Output_Select)RTSI_Output_Select);
         board->RTSI_Trig_Direction.writeRTSI7_Pin_Dir((tMSeries::tRTSI_Trig_Direction::tRTSI7_Pin_Dir)enable);
         break;

      //
      // Set PFI terminal
      //
      case kPFI0:
         board->PFI_Output_Select_1.writePFI0_Output_Select ((tMSeries::tPFI_Output_Select_1::tPFI0_Output_Select)PFI_Output_Select);
         board->IO_Bidirection_Pin.writePFI0_Pin_Dir((tMSeries::tIO_Bidirection_Pin::tPFI0_Pin_Dir)enable);
         break;
      
      case kPFI1:
         board->PFI_Output_Select_1.writePFI1_Output_Select ((tMSeries::tPFI_Output_Select_1::tPFI1_Output_Select)PFI_Output_Select);
         board->IO_Bidirection_Pin.writePFI1_Pin_Dir((tMSeries::tIO_Bidirection_Pin::tPFI1_Pin_Dir)enable);
         break;
      
      case kPFI2:
         board->PFI_Output_Select_1.writePFI2_Output_Select ((tMSeries::tPFI_Output_Select_1::tPFI2_Output_Select)PFI_Output_Select);
         board->IO_Bidirection_Pin.writePFI2_Pin_Dir((tMSeries::tIO_Bidirection_Pin::tPFI2_Pin_Dir)enable);
         break;
      
      case kPFI3:
         board->PFI_Output_Select_2.writePFI3_Output_Select ((tMSeries::tPFI_Output_Select_2::tPFI3_Output_Select)PFI_Output_Select);
         board->IO_Bidirection_Pin.writePFI3_Pin_Dir((tMSeries::tIO_Bidirection_Pin::tPFI3_Pin_Dir)enable);
         break;
      
      case kPFI4:
         board->PFI_Output_Select_2.writePFI4_Output_Select ((tMSeries::tPFI_Output_Select_2::tPFI4_Output_Select)PFI_Output_Select);
         board->IO_Bidirection_Pin.writePFI4_Pin_Dir((tMSeries::tIO_Bidirection_Pin::tPFI4_Pin_Dir)enable);
         break;
      
      case kPFI5:
         board->PFI_Output_Select_2.writePFI5_Output_Select ((tMSeries::tPFI_Output_Select_2::tPFI5_Output_Select)PFI_Output_Select);
         board->IO_Bidirection_Pin.writePFI5_Pin_Dir((tMSeries::tIO_Bidirection_Pin::tPFI5_Pin_Dir)enable);      
         break;
      
      case kPFI6:
         board->PFI_Output_Select_3.writePFI6_Output_Select ((tMSeries::tPFI_Output_Select_3::tPFI6_Output_Select)PFI_Output_Select);
         board->IO_Bidirection_Pin.writePFI6_Pin_Dir((tMSeries::tIO_Bidirection_Pin::tPFI6_Pin_Dir)enable);      
         break;
      
      case kPFI7:
         board->PFI_Output_Select_3.writePFI7_Output_Select ((tMSeries::tPFI_Output_Select_3::tPFI7_Output_Select)PFI_Output_Select);
         board->IO_Bidirection_Pin.writePFI7_Pin_Dir((tMSeries::tIO_Bidirection_Pin::tPFI7_Pin_Dir)enable);  
         break;
      
      case kPFI8:
         board->PFI_Output_Select_3.writePFI8_Output_Select ((tMSeries::tPFI_Output_Select_3::tPFI8_Output_Select)PFI_Output_Select);
         board->IO_Bidirection_Pin.writePFI8_Pin_Dir((tMSeries::tIO_Bidirection_Pin::tPFI8_Pin_Dir)enable);  
         break;
      
      case kPFI9:
         board->PFI_Output_Select_4.writePFI9_Output_Select ((tMSeries::tPFI_Output_Select_4::tPFI9_Output_Select)PFI_Output_Select);
         board->IO_Bidirection_Pin.writePFI9_Pin_Dir((tMSeries::tIO_Bidirection_Pin::tPFI9_Pin_Dir)enable);  
         break; 
      
      case kPFI10:      
         board->PFI_Output_Select_4.writePFI10_Output_Select ((tMSeries::tPFI_Output_Select_4::tPFI10_Output_Select)PFI_Output_Select);
         board->IO_Bidirection_Pin.writePFI10_Pin_Dir((tMSeries::tIO_Bidirection_Pin::tPFI10_Pin_Dir)enable);  
         break;
      
      case kPFI11:
         board->PFI_Output_Select_4.writePFI11_Output_Select ((tMSeries::tPFI_Output_Select_4::tPFI11_Output_Select)PFI_Output_Select);
         board->IO_Bidirection_Pin.writePFI11_Pin_Dir((tMSeries::tIO_Bidirection_Pin::tPFI11_Pin_Dir)enable);  
         break;
      
      case kPFI12:
         board->PFI_Output_Select_5.writePFI12_Output_Select ((tMSeries::tPFI_Output_Select_5::tPFI12_Output_Select)PFI_Output_Select);
         board->IO_Bidirection_Pin.writePFI12_Pin_Dir((tMSeries::tIO_Bidirection_Pin::tPFI12_Pin_Dir)enable);  
         break;
      
      case kPFI13:
         board->PFI_Output_Select_5.writePFI13_Output_Select ((tMSeries::tPFI_Output_Select_5::tPFI13_Output_Select)PFI_Output_Select);
         board->IO_Bidirection_Pin.writePFI13_Pin_Dir((tMSeries::tIO_Bidirection_Pin::tPFI13_Pin_Dir)enable);  
         break;
         
      case kPFI14:
         board->PFI_Output_Select_5.writePFI14_Output_Select ((tMSeries::tPFI_Output_Select_5::tPFI14_Output_Select)PFI_Output_Select);
         board->IO_Bidirection_Pin.writePFI14_Pin_Dir((tMSeries::tIO_Bidirection_Pin::tPFI14_Pin_Dir)enable);  
         break;
      
      case kPFI15:
         board->PFI_Output_Select_6.writePFI15_Output_Select ((tMSeries::tPFI_Output_Select_6::tPFI15_Output_Select)PFI_Output_Select);
         board->IO_Bidirection_Pin.writePFI15_Pin_Dir((tMSeries::tIO_Bidirection_Pin::tPFI15_Pin_Dir)enable);  
         break;
      
      default:
         return;
   }
   
   //
   // If this is a RTSI terminal, check if the RTSI MUX need to be set.
   //
   if (isRTSITerminal)
   {
      switch (RTSI_Output_Select)
      {
         case kRTSI_BRD_0:
            board->RTSI_Shared_MUX.writeRTSI_Shared_MUX_0_Output_Select((tMSeries::tRTSI_Shared_MUX::tRTSI_Shared_MUX_0_Output_Select)RTSI_Shared_MUX_Output_Select);
            break;
         
         case kRTSI_BRD_1:
            board->RTSI_Shared_MUX.writeRTSI_Shared_MUX_1_Output_Select((tMSeries::tRTSI_Shared_MUX::tRTSI_Shared_MUX_1_Output_Select)RTSI_Shared_MUX_Output_Select);
            break;
         
         case kRTSI_BRD_2:
            board->RTSI_Shared_MUX.writeRTSI_Shared_MUX_2_Output_Select((tMSeries::tRTSI_Shared_MUX::tRTSI_Shared_MUX_2_Output_Select)RTSI_Shared_MUX_Output_Select);
            break;
         
         case kRTSI_BRD_3:
            board->RTSI_Shared_MUX.writeRTSI_Shared_MUX_3_Output_Select((tMSeries::tRTSI_Shared_MUX::tRTSI_Shared_MUX_3_Output_Select)RTSI_Shared_MUX_Output_Select);
            break;
         
         default:
            // do nothing
            break;
      }
   }
}
