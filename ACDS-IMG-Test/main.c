//ACDS Test
//Denise Thorsen
//2014-07-07

#include <string.h>
#include <ctl.h>
#include <msp430.h>
#include <ARCbus.h>
#include <terminal.h>
#include <UCA1_uart.h>
#include <stdio.h>
#include <Error.h>
#include "ACDS.h"
#include "IMG.h"
#include "pins.h"

//Define three task structures in array tasks (what are these tasks?)
CTL_TASK_t IMG_e_task;
CTL_TASK_t ACDS_e_task;
CTL_TASK_t SUB_e_task;
CTL_TASK_t terminal_task;

//Define stacks for each tasks, STACKSize is the middle number
unsigned IMG_e_stack[1+500+1];
unsigned ACDS_e_stack[1+500+1];
unsigned SUB_e_stack[1+500+1];
unsigned terminal_stack[1+500+1];

//make printf and friends to send chars out UCA1 uart
int __putchar(int c){
  //don't print if async connection is open
  if(!async_isOpen()){
    return UCA1_TxChar(c);
  }else{
    return EOF;
  }
}

//set scanf and friends to read chars from UAC1 uart
int __getchar(void){
    return UCA1_Getc();
}

#define stacksize(s) (sizeof(s)/sizeof(s[0])-2)
#define stackinit(s) (s[0]=s[sizeof(s)/sizeof(s[0])-1]=0xfeed)

int main(void)
{
  //Do this first
  ARC_setup();

  //setup UCA1 uart
  UCA1_init_UART(UART_PORT,UART_TX_PIN_NUM,UART_RX_PIN_NUM);

  //setup buss interface with ACDS address as primary
  initARCbus(BUS_ADDR_ACDS);

  //setup command parse for ACDS
  BUS_register_cmd_callback(&ACDS_parse);

  //Setup ADDR1 for IMG
  BUS_I2C_aux_addr(BUS_ADDR_IMG,CMD_PARSE_ADDR1);

  //setup command parse for IMG
  BUS_register_cmd_callback(&IMG_parse);

  //setup P7 I/O output
  P7OUT = BUS_ADDR_ACDS^BUS_ADDR_IMG;
  P7DIR = 0xFF;
  P7SEL0 = 0;

  //initialize stacks
  stackinit(IMG_e_stack); // put marker values at the words before/after the stack
  stackinit(ACDS_e_stack); // put marker values at the words before/after the stack
  stackinit(SUB_e_stack); // put marker values at the words before/after the stack
  stackinit(terminal_stack); // put marker values at the words before/after the stack

//create tasks
  ctl_task_run(&IMG_e_task, BUS_PRI_LOW, IMG_events, NULL, "IMG_events",stacksize(IMG_e_stack),IMG_e_stack+1,0);
  ctl_task_run(&ACDS_e_task, BUS_PRI_LOW+1, ACDS_events, NULL, "ACDS_events", stacksize(ACDS_e_stack),ACDS_e_stack+1,0);
  ctl_task_run(&terminal_task, BUS_PRI_NORMAL, terminal, "Test IMG ACDS code", "terminal", stacksize(terminal_stack),terminal_stack+1,0);
  ctl_task_run(&SUB_e_task, BUS_PRI_HIGH, sub_events, NULL, "sub_events", stacksize(SUB_e_stack),SUB_e_stack+1,0);
   
 //Call mainLoop to initialize the ARCbus task and drop the idle task priority to zero allowing other tasks to run.  This is the idle loop.
  mainLoop();

}

