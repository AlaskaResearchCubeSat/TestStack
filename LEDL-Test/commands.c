#include <msp430.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctl.h>
#include <terminal.h>
#include <ARCbus.h>
#include <UCA1_uart.h>
#include <crc.h>
#include <commandLib.h>
#include "LEDL.h"
#include <Error.h>
#include "LEDL_errors.h"

//Retreive status LEDL
int statusCmd(char *argv[],unsigned short argc){

  int i;
   //flash lower four bits IMG address to P7 LED's 10 times
   P7OUT=BUS_ADDR_LEDL;
   for (i=0;i<10;i++){
	ctl_timeout_wait(ctl_get_current_time()+102);
	P7OUT=~(BUS_ADDR_LEDL);
	ctl_timeout_wait(ctl_get_current_time()+102);
	P7OUT=(BUS_ADDR_LEDL);
   }
  //Need to send back status through terminal.
  
  P7OUT=BUS_ADDR_LEDL; //finish present CDH address
  printf("LEDL On.  Check LEDs: flashing 0x%02x - 0x%02x\r\n", BUS_ADDR_LEDL, ~(BUS_ADDR_LEDL));
}

//reset a MSP430 on command
int resetCmd(char **argv,unsigned short argc){
  //force user to pass no arguments to prevent unwanted resets
  if(argc!=0){
    printf("Error : %s takes no arguments\r\n",argv[0]);
    return -1;
  }
  //print reset message
  puts("Initiating reset\r\n");
  //wait for UART buffer to empty
  while(UCA1_CheckBusy());
  //write to WDTCTL without password causes PUC
  WDTCTL=0;
  //Never reached due to reset
  puts("Error : Reset Failed!\r");
  return 0;
}

int LEDLCmd(char **argv, unsigned short argc)
{
  printf("You are in LEDL \r\n\t");
  return 0;
}

//table of commands with help
const CMD_SPEC cmd_tbl[]={
                    {"LEDL", "\r\n\t", LEDLCmd},
                    {"help"," [command]",helpCmd},
                    {"reset","\r\n\t""Reset the MSP430",resetCmd},
                    //{"StatusIMG","\r\n\t""Get IMG status",statusCmd},
                   //end of list

                   {NULL,NULL,NULL}};

