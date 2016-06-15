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
#include "IMG.h"
#include <Error.h>
#include "IMG_errors.h"

//Turn on IMG
int onCmd(char *argv[],unsigned short argc){
  //output lower four bits COMM address (Ox13) to P7 LED's

  P7OUT=BUS_ADDR_IMG;

  //Perhaps should set a register here that says we are commanded on.
  
  printf("IMG On.  Check LEDs: 0bxxxx0011\r\n");
}

//Turn off IMG
int offCmd(char *argv[],unsigned short argc){
   //output lower four bits COMM address (Ox13) to P7 LED's
  P7OUT=0;

  //Perhaps should set a register here that says we are commanded off.
  
  printf("IMG Off.  Check LEDs: 0bxxxx0000\r\n");
}

//Retreive status IMG
int statusCmd(char *argv[],unsigned short argc){

  int i;
   //flash lower four bits IMG address to P7 LED's 10 times
   P7OUT=BUS_ADDR_IMG;
   for (i=0;i<10;i++){
	ctl_timeout_wait(ctl_get_current_time()+102);
	P7OUT=~(BUS_ADDR_IMG);
	ctl_timeout_wait(ctl_get_current_time()+102);
	P7OUT=(BUS_ADDR_IMG);
   }
  //Need to send back status through terminal.
  
  P7OUT=BUS_ADDR_IMG; //finish present CDH address
  printf("IMG On.  Check LEDs: flashing 0bxxxx0011 - 0bxxxx1100\r\n");
}



//table of commands with help
const CMD_SPEC cmd_tbl[]={{"help"," [command]",helpCmd},
                    ARC_COMMANDS,CTL_COMMANDS,ERROR_COMMANDS,
                    {"OnIMG","[bgnd|stop]\r\n\t""Command ON IMG",onCmd},
                    {"OffIMG","port [port ...]\r\n\t""Command OFF IMG",offCmd},
                    {"StatusIMG","\r\n\t""Get IMG status",statusCmd},
                   //end of list

                   {NULL,NULL,NULL}};

