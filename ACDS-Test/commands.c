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
#include "ACDS.h"
#include <Error.h>
#include "ACDS_errors.h"

//Turn on ACDS
int onCmd(char *argv[],unsigned short argc){
  //output lower four bits COMM address (Ox13) to P7 LED's

  P7OUT=BUS_ADDR_ACDS;
  printf("ACDS On.  Check LEDs: 0x%02x\r\n", BUS_ADDR_ACDS);
}

//Turn off ACDS
int offCmd(char *argv[],unsigned short argc){
  P7OUT=0;
  printf("ACDS Off.  Check LEDs: 0x00\r\n");
}

//Retreive status ACDS
int statusCmd(char *argv[],unsigned short argc){

  int i;
   //flash lower four bits ACDS address to P7 LED's 10 times
   P7OUT=BUS_ADDR_ACDS;
   for (i=0;i<10;i++){
	ctl_timeout_wait(ctl_get_current_time()+102);
	P7OUT=~(BUS_ADDR_ACDS);
	ctl_timeout_wait(ctl_get_current_time()+102);
	P7OUT=(BUS_ADDR_ACDS);
   }
  //Need to send back status through terminal.
  
  P7OUT=BUS_ADDR_ACDS; //finish present CDH address
  printf("ACDS On.  Check LEDs: flashing 0x%02x - 0x%0x\r\n", BUS_ADDR_ACDS, ~(BUS_ADDR_ACDS));
}


//table of commands with help
const CMD_SPEC cmd_tbl[]={{"help"," [command]",helpCmd},
                    ARC_COMMANDS,
                    {"OnIMG","[bgnd|stop]\r\n\t""Command ON ACDS",onCmd},
                    {"OffIMG","port [port ...]\r\n\t""Command OFF ACDS",offCmd},
                    {"StatusIMG","\r\n\t""Get ACDS status",statusCmd},
                   //end of list

                   {NULL,NULL,NULL}};

