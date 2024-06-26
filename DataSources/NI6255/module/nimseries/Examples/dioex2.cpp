//
//  doiex2.cpp --
//
//      Digital read/write
//      Writes a u8 out on port 1
//      Reads a u8 in on port 2
//      Before running, wire PFI0 to PFI8, PFI1 to PFI9, ..., and PFI7 to PFI15
//
//  $DateTime: 2006/10/24 23:40:45 $
//

#include <stdio.h>

#ifndef ___tMSeries_h___
 #include "tMSeries.h"
#endif

void test(iBus *bus)
{

    // create register map

    tAddressSpace  bar1;
    tMSeries *board;

    bar1 = bus->createAddressSpace(kPCI_BAR1);
    board = new tMSeries(bar1);


    // ---- Start DIO task, Port 1 DO, Port 2 DI ----
    board->IO_Bidirection_Pin.writeRegister (0x00FF);
    board->PFI_Output_Select_1.writeRegister (0x4210);
    board->PFI_Output_Select_2.writeRegister (0x4210);
    board->PFI_Output_Select_3.writeRegister (0x210);
    board->PFI_Output_Select_4.writeRegister (0x000000);
    board->PFI_Output_Select_5.writeRegister (0x000000);
    board->PFI_Output_Select_6.writeRegister (0x00);

    /* The following does the same pin dircetion settings, but line by line
    board->IO_Bidirection_Pin.writePFI0_Pin_Dir (kPFI0_Pin_DirOutput);
    board->PFI_Output_Select_1.writePFI0_Output_Select (kPFI0_Output_SelectPFI_DO);
    board->IO_Bidirection_Pin.writePFI1_Pin_Dir (kPFI1_Pin_DirOutput);
    board->PFI_Output_Select_1.writePFI1_Output_Select (kPFI1_Output_SelectPFI_DO);
    board->IO_Bidirection_Pin.writePFI2_Pin_Dir (kPFI2_Pin_DirOutput);
    board->PFI_Output_Select_1.writePFI2_Output_Select (kPFI2_Output_SelectPFI_DO);
    board->IO_Bidirection_Pin.writePFI3_Pin_Dir (kPFI3_Pin_DirOutput);
    board->PFI_Output_Select_2.writePFI3_Output_Select (kPFI3_Output_SelectPFI_DO);
    board->IO_Bidirection_Pin.writePFI4_Pin_Dir (kPFI4_Pin_DirOutput);
    board->PFI_Output_Select_2.writePFI4_Output_Select (kPFI4_Output_SelectPFI_DO);
    board->IO_Bidirection_Pin.writePFI5_Pin_Dir (kPFI5_Pin_DirOutput);
    board->PFI_Output_Select_2.writePFI5_Output_Select (kPFI5_Output_SelectPFI_DO);
    board->IO_Bidirection_Pin.writePFI6_Pin_Dir (kPFI6_Pin_DirOutput);
    board->PFI_Output_Select_3.writePFI6_Output_Select (kPFI6_Output_SelectPFI_DO);
    board->IO_Bidirection_Pin.writePFI7_Pin_Dir (kPFI7_Pin_DirOutput);
    board->PFI_Output_Select_3.writePFI7_Output_Select (kPFI7_Output_SelectPFI_DO);

    board->IO_Bidirection_Pin.writePFI8_Pin_Dir (kPFI8_Pin_DirInput);
    board->IO_Bidirection_Pin.writePFI9_Pin_Dir (kPFI9_Pin_DirInput);
    board->IO_Bidirection_Pin.writePFI10_Pin_Dir (kPFI10_Pin_DirInput);
    board->IO_Bidirection_Pin.writePFI11_Pin_Dir (kPFI11_Pin_DirInput);
    board->IO_Bidirection_Pin.writePFI12_Pin_Dir (kPFI12_Pin_DirInput);
    board->IO_Bidirection_Pin.writePFI13_Pin_Dir (kPFI13_Pin_DirInput);
    board->IO_Bidirection_Pin.writePFI14_Pin_Dir (kPFI14_Pin_DirInput);
    board->IO_Bidirection_Pin.writePFI15_Pin_Dir (kPFI15_Pin_DirInput);
    */


    // ---- Write to port ----

    board->PFI_DO.writeLowerPort (0x55);

    // ---- Read-back value ----

    u8 value;
    value = (u8) (board->PFI_DI.readRegister() >> 8);

    printf ("port2 value: 0x%02X\n", value);

    //cleanup
    delete board;
    bus->destroyAddressSpace(bar1);
}
