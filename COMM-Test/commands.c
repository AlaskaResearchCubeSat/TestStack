#include <msp430.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctl.h>
#include <terminal.h>
#include <ARCbus.h>
#include <limits.h>
#include <UCA1_uart.h>
#include <crc.h>
#include <commandLib.h>
#include "COMM.h"
#include <Error.h>
#include "COMM_errors.h"

int ground_cmd(char *argv[],unsigned short argc){
  unsigned char addr,id;
  unsigned long tmp;
  int i,ret;
  unsigned char *ptr,buff[BUS_I2C_HDR_LEN+BUS_I2C_MAX_PACKET_LEN+BUS_I2C_CRC_LEN];
  char *end;
  //check number of arguments
  if(argc<2){
    printf("Error : %s requires at least 2 arguments\r\n",argv[0]);
    return -1;
  }
  //get address
  addr=getI2C_addr(argv[1],0,busAddrSym);
  if(addr==0xFF){
    return 1;
  }
  //get packet ID
  tmp=strtoul(argv[2],&end,0);
  if(end==argv[2]){
      printf("Error : could not parse element \"%s\".\r\n",argv[2]);
      return 2;
  }
  if(*end!=0){
    printf("Error : unknown sufix \"%s\" at end of element \"%s\"\r\n",end,argv[2]);
    return 3;
  }
  if(tmp>UCHAR_MAX){
    printf("Error : vlaue \"%s\" too large\r\n",argv[2]);
    return 4;
  }
  //store in ID
  id=tmp;
  //setup packet 
  
  //init command
  ptr=BUS_cmd_init(buff,id);
  for(i=3;i<=argc;i++){
    //get packet ID
    tmp=strtoul(argv[i],&end,0);
    if(end==argv[i]){
        printf("Error : could not parse element \"%s\".\r\n",argv[i]);
        return 2;
    }
    if(*end!=0){
      printf("Error : unknown sufix \"%s\" at end of element \"%s\"\r\n",end,argv[i]);
      return 3;
    }
    if(tmp>UCHAR_MAX){
      printf("Error : vlaue \"%s\" too large\r\n",argv[i]);
      return 4;
    }
    //store in buffer
    *ptr++=tmp;
  }
  //print command
  printf("Sending %s command (0x%02X) :\r\n\t",BUS_cmdtostr(id),id);
  for(i=0;i<argc-2+BUS_I2C_HDR_LEN;i++){
    printf("0x%02X ",buff[i]);
  }
  //newline after command
  printf("\r\n");  
  //send packet
  ret=BUS_cmd_tx(addr,buff,argc-2,0);
  //check for success
  if(ret==RET_SUCCESS){
    printf("Ground Command sent successfully\r\n");
  }else{
    printf("Error : could not send command %s (%i)\r\n",BUS_error_str(ret),ret);
  }
  return 0;
}

int COMMCmd(char **argv, unsigned short argc)
{
  printf("You are in COMM\r\n\t");
  return 0;
}

//table of commands with help
const CMD_SPEC cmd_tbl[]={
                    {"COMM", "\r\n\t", COMMCmd},
                    {"help"," [command]",helpCmd},
                    //ARC_COMMANDS,CTL_COMMANDS,ERROR_COMMANDS,
                    {"gnd","addr cmd""\r\n\t""Simulate ground commands",ground_cmd},
                   //end of list

                   {NULL,NULL,NULL}};

