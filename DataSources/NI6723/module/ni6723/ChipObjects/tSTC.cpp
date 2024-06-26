// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// This file is autogenerated!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#ifndef ___tSTC_h___
#include "tSTC.h"

#endif

tSTC::tSTC(tBusSpaceReference addrSpace, nMDBG::tStatus2* s)

{
   _addressOffset = 0;
   _addrSpace = addrSpace;
   _deallocateBus = kFalse;

   if (s && s->isFatal()) return;
   _initialize(s);
}


void tSTC::reset(nMDBG::tStatus2* s)
{
   if (s && s->isFatal()) return;

   AI_Command_1.setRegister(0x0, s);
   AI_Command_2.setRegister(0x0, s);
   Interrupt_A_Ack.setRegister(0x0, s);
   Interrupt_A_Enable.setRegister(0x0, s);
   Second_Irq_A_Enable.setRegister(0x0, s);
   AI_Personal.setRegister(0x0, s);
   AI_Mode_1.setRegister(0x0, s);
   AI_Mode_2.setRegister(0x0, s);
   AI_Output_Control.setRegister(0x0, s);
   AI_Mode_3.setRegister(0x0, s);
   AI_START_STOP_Select.setRegister(0x0, s);
   AI_Trigger_Select.setRegister(0x0, s);
   AI_SI_Load_A.setRegister(0x0, s);
   AI_SI_Load_B.setRegister(0x0, s);
   AI_SI2_Load_A.setRegister(0x0, s);
   AI_SI2_Load_B.setRegister(0x0, s);
   AI_SC_Load_A.setRegister(0x0, s);
   AI_SC_Load_B.setRegister(0x0, s);
   AI_DIV_Load_A.setRegister(0x0, s);
   AO_Personal.setRegister(0x0, s);
   AO_Command_1.setRegister(0x0, s);
   AO_Command_2.setRegister(0x0, s);
   Interrupt_B_Ack.setRegister(0x0, s);
   Interrupt_B_Enable.setRegister(0x0, s);
   Second_Irq_B_Enable.setRegister(0x0, s);
   AO_Mode_1.setRegister(0x0, s);
   AO_Mode_2.setRegister(0x0, s);
   AO_Output_Control.setRegister(0x0, s);
   AO_Mode_3.setRegister(0x0, s);
   AO_START_Select.setRegister(0x0, s);
   AO_Trigger_Select.setRegister(0x0, s);
   AO_UI_Load_A.setRegister(0x0, s);
   AO_UI_Load_B.setRegister(0x0, s);
   AO_UI2_Load_A.setRegister(0x0, s);
   AO_UI2_Load_B.setRegister(0x0, s);
   AO_BC_Load_A.setRegister(0x0, s);
   AO_BC_Load_B.setRegister(0x0, s);
   AO_UC_Load_A.setRegister(0x0, s);
   AO_UC_Load_B.setRegister(0x0, s);
   DIO_Control.setRegister(0x0, s);
   DIO_Output.setRegister(0x0, s);
   G0_Mode.setRegister(0x0, s);
   G1_Mode.setRegister(0x0, s);
   G0_Command.setRegister(0x0, s);
   G1_Command.setRegister(0x0, s);
   G0_Input_Select.setRegister(0x0, s);
   G1_Input_Select.setRegister(0x0, s);
   G0_Load_A.setRegister(0x0, s);
   G0_Autoincrement.setRegister(0x0, s);
   G0_Load_B.setRegister(0x0, s);
   G1_Load_A.setRegister(0x0, s);
   G1_Autoincrement.setRegister(0x0, s);
   G1_Load_B.setRegister(0x0, s);
   Generic_Control.setRegister(0x0, s);
   IO_Bidirection_Pin.setRegister(0x0, s);
   Analog_Trigger_Etc.setRegister(0x0, s);
   Interrupt_Control.setRegister(0x0, s);
   Clock_and_FOUT.setRegister(0x0, s);
   Joint_Reset.setRegister(0x0, s);
   RTSI_Trig_Direction.setRegister(0x0, s);
   RTSI_Trig_A_Output.setRegister(0x0, s);
   RTSI_Trig_B_Output.setRegister(0x0, s);
   RTSI_Board.setRegister(0x0, s);
   Write_Strobe_0.setRegister(0x0, s);
   Write_Strobe_1.setRegister(0x0, s);
   Write_Strobe_2.setRegister(0x0, s);
   Write_Strobe_3.setRegister(0x0, s);
   AI_DIV_Save.setRegister(0x0, s);
   AI_SC_Save.setRegister(0x0, s);
   AI_SI_Save.setRegister(0x0, s);
   AI_SI2_Save.setRegister(0x0, s);
   AI_Status_1.setRegister(0x0, s);
   AI_Status_2.setRegister(0x0, s);
   AO_BC_Save.setRegister(0x0, s);
   AO_Status_1.setRegister(0x0, s);
   AO_Status_2.setRegister(0x0, s);
   AO_UC_Save.setRegister(0x0, s);
   AO_UI_Save.setRegister(0x0, s);
   AO_UI2_Save.setRegister(0x0, s);
   DIO_Parallel_Input.setRegister(0x0, s);
   DIO_Serial_Input.setRegister(0x0, s);
   G0_HW_Save.setRegister(0x0, s);
   G0_Save.setRegister(0x0, s);
   G1_HW_Save.setRegister(0x0, s);
   G1_Save.setRegister(0x0, s);
   G_Status.setRegister(0x0, s);
   Joint_Status_1.setRegister(0x0, s);
   Joint_Status_2.setRegister(0x0, s);
}

void tSTC::_initialize(nMDBG::tStatus2* s)
{
   if (s && s->isFatal()) return;



   //----------------------------------------
   // set register maps of all registers
   //----------------------------------------
   AI_Command_1.setRegisterMap(this);
   AI_Command_2.setRegisterMap(this);
   Interrupt_A_Ack.setRegisterMap(this);
   Interrupt_A_Enable.setRegisterMap(this);
   Second_Irq_A_Enable.setRegisterMap(this);
   AI_Personal.setRegisterMap(this);
   AI_Mode_1.setRegisterMap(this);
   AI_Mode_2.setRegisterMap(this);
   AI_Output_Control.setRegisterMap(this);
   AI_Mode_3.setRegisterMap(this);
   AI_START_STOP_Select.setRegisterMap(this);
   AI_Trigger_Select.setRegisterMap(this);
   AI_SI_Load_A.setRegisterMap(this);
   AI_SI_Load_B.setRegisterMap(this);
   AI_SI2_Load_A.setRegisterMap(this);
   AI_SI2_Load_B.setRegisterMap(this);
   AI_SC_Load_A.setRegisterMap(this);
   AI_SC_Load_B.setRegisterMap(this);
   AI_DIV_Load_A.setRegisterMap(this);
   AO_Personal.setRegisterMap(this);
   AO_Command_1.setRegisterMap(this);
   AO_Command_2.setRegisterMap(this);
   Interrupt_B_Ack.setRegisterMap(this);
   Interrupt_B_Enable.setRegisterMap(this);
   Second_Irq_B_Enable.setRegisterMap(this);
   AO_Mode_1.setRegisterMap(this);
   AO_Mode_2.setRegisterMap(this);
   AO_Output_Control.setRegisterMap(this);
   AO_Mode_3.setRegisterMap(this);
   AO_START_Select.setRegisterMap(this);
   AO_Trigger_Select.setRegisterMap(this);
   AO_UI_Load_A.setRegisterMap(this);
   AO_UI_Load_B.setRegisterMap(this);
   AO_UI2_Load_A.setRegisterMap(this);
   AO_UI2_Load_B.setRegisterMap(this);
   AO_BC_Load_A.setRegisterMap(this);
   AO_BC_Load_B.setRegisterMap(this);
   AO_UC_Load_A.setRegisterMap(this);
   AO_UC_Load_B.setRegisterMap(this);
   DIO_Control.setRegisterMap(this);
   DIO_Output.setRegisterMap(this);
   G0_Mode.setRegisterMap(this);
   G1_Mode.setRegisterMap(this);
   G0_Command.setRegisterMap(this);
   G1_Command.setRegisterMap(this);
   G0_Input_Select.setRegisterMap(this);
   G1_Input_Select.setRegisterMap(this);
   G0_Load_A.setRegisterMap(this);
   G0_Autoincrement.setRegisterMap(this);
   G0_Load_B.setRegisterMap(this);
   G1_Load_A.setRegisterMap(this);
   G1_Autoincrement.setRegisterMap(this);
   G1_Load_B.setRegisterMap(this);
   Generic_Control.setRegisterMap(this);
   IO_Bidirection_Pin.setRegisterMap(this);
   Analog_Trigger_Etc.setRegisterMap(this);
   Interrupt_Control.setRegisterMap(this);
   Clock_and_FOUT.setRegisterMap(this);
   Joint_Reset.setRegisterMap(this);
   RTSI_Trig_Direction.setRegisterMap(this);
   RTSI_Trig_A_Output.setRegisterMap(this);
   RTSI_Trig_B_Output.setRegisterMap(this);
   RTSI_Board.setRegisterMap(this);
   Write_Strobe_0.setRegisterMap(this);
   Write_Strobe_1.setRegisterMap(this);
   Write_Strobe_2.setRegisterMap(this);
   Write_Strobe_3.setRegisterMap(this);
   AI_DIV_Save.setRegisterMap(this);
   AI_SC_Save.setRegisterMap(this);
   AI_SI_Save.setRegisterMap(this);
   AI_SI2_Save.setRegisterMap(this);
   AI_Status_1.setRegisterMap(this);
   AI_Status_2.setRegisterMap(this);
   AO_BC_Save.setRegisterMap(this);
   AO_Status_1.setRegisterMap(this);
   AO_Status_2.setRegisterMap(this);
   AO_UC_Save.setRegisterMap(this);
   AO_UI_Save.setRegisterMap(this);
   AO_UI2_Save.setRegisterMap(this);
   DIO_Parallel_Input.setRegisterMap(this);
   DIO_Serial_Input.setRegisterMap(this);
   G0_HW_Save.setRegisterMap(this);
   G0_Save.setRegisterMap(this);
   G1_HW_Save.setRegisterMap(this);
   G1_Save.setRegisterMap(this);
   G_Status.setRegisterMap(this);
   Joint_Status_1.setRegisterMap(this);
   Joint_Status_2.setRegisterMap(this);

   reset(s);
}

tSTC::~tSTC()
{
}



// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// This file is autogenerated!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!

