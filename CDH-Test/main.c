//CDH Test
//Denise Thorsen
//2014-07-10

#include <msp430.h>
#include <string.h>
#include <ctl.h>
#include <ARCbus.h>
#include <terminal.h>
#include <UCA1_uart.h>
#include <stdio.h>
#include <Error.h>
#include "CDH.h"
#include "pins.h"
#include "CDH_errors.h"
#include <SDlib.h>

CTL_TASK_t tasks[3];

//stacks for tasks,
unsigned stack1[1+500+1];
unsigned stack2[1+500+1];
unsigned stack3[1+500+1];

//set printf and friends to send chars out UCA1 uart
int __putchar(int c){
  //don't print if async connection is open
  if(!async_isOpen()){
    return UCA1_TxChar(c);
  }else{
    return EOF;
  }
}

//set scanf and friends to read chars from UCA1 uart
int __getchar(void){
    return UCA1_Getc();
}

int main(void){
  //======================[setup AUX supplies]======================
  //Setup charging for RTC backup cap
  if(AUXCTL0&LOCKAUX){
    //unlock AUX registers
    AUXCTL0_H=AUXKEY_H;
    //disable all supplies but VCC
    AUXCTL1=AUX2MD|AUX1MD|AUX0MD|AUX0OK;
    //setup charging for AUXVCC3
    AUX3CHCTL=AUXCHKEY|AUXCHV_1|AUXCHC_1|AUXCHEN;
    //clear LOCKAUX bit
    AUXCTL0=AUXKEY;
    //lock AUX registers
    AUXCTL0_H=0;
  }
  //===================[setup clocks and supplies]===================
  ARC_setup();
  
  //====================[register error handler]====================

  err_register_handler(CDH_ERROR_MIN,CDH_ERROR_MAX,CDH_err_decode,ERR_FLAGS_SUBSYSTEM);

  //==================[setup subsystem Peripherals]==================
  
  //setup reset pins
  P6OUT=0;      //all pins low
  P6DIR=0xFF;   //all pins outputs
  P6SEL0=0;
  P6SEL1=0;

  //setup timer for beacon packets
  init_CDH_timer();

  //initialize SD card pins
  mmcInit_msp();

  //setup UCA1 UART
  UCA1_init_UART(UART_PORT,UART_TX_PIN_NUM,UART_RX_PIN_NUM);

  //===============[setup ARC Peripherals and tasking]===============
  initARCbus(BUS_ADDR_CDH);

  //setup command recive
  BUS_register_cmd_callback(&CDH_parse);
  
  //=====================[setup subsystem tasks]=====================
  
  //initialize stacks
  memset(stack1, 0xcd, sizeof(stack1));  // write known values into the stack
  stack1[0]=stack1[sizeof(stack1)/sizeof(stack1[0])-1]=0xfeed; // put marker values at the words before/after the stack
 
  memset(stack2, 0xcd, sizeof(stack2));  // write known values into the stack
  stack2[0]=stack2[sizeof(stack2)/sizeof(stack2[0])-1]=0xfeed; // put marker values at the words before/after the stack

  memset(stack3, 0xcd, sizeof(stack3));  // write known values into the stack
  stack3[0]=stack3[sizeof(stack3)/sizeof(stack3[0])-1]=0xfeed; // put marker values at the words before/after the stack

//create tasks
  ctl_task_run(&tasks[0], BUS_PRI_LOW, cmd_parse, NULL, "cmd_parse", sizeof(stack1)/sizeof(stack1[0])-2,stack1+1,0);
  ctl_task_run(&tasks[1], BUS_PRI_LOW, terminal, "Test CDH code", "terminal", sizeof(stack2)/sizeof(stack2[0])-2,stack2+1,0);
  ctl_task_run(&tasks[2], BUS_PRI_NORMAL, sub_events, NULL, "sub_events", sizeof(stack3)/sizeof(stack3[0])-2,stack3+1,0);
  
  //====================[Final subsystem setup]====================

  //start CDH timer for status requests
  start_CDH_timer();

  //set LED's for shifting
  if(!P7OUT){
    P7OUT = BIT4;
  }
  P7DIR = 0xFF;
  P7SEL0 = 0;


  //=======================[enter Idle loop]=======================
  //initialize the ARCbus task and drop the idle task priority to zero allowing other tasks to run
  mainLoop();

}
