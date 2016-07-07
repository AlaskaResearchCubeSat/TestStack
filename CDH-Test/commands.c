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
#include "CDH.h"
#include <Error.h>
#include "CDH_errors.h"

int statCmd(char **argv,unsigned short argc)
{
  int i;
  unsigned char *ptr;
  //Read status
  ctl_events_set_clear(&cmd_parse_evt,CMD_PARSE_GET_STAT_CMD,0);
  //wait a bit so status can be returned
  ctl_timeout_wait(ctl_get_current_time()+3*1024);
  //check status
  if(system_stat.flags&STAT_ALL_VALID==STAT_ALL_VALID){
    printf("All subsystems reported status\r\n");
  }else{
    if(!(system_stat.flags&STAT_EPS_VALID)){
      printf("No Status Info for EPS\r\n");
    }
    if(!(system_stat.flags&STAT_LEDL_VALID)){
      printf("No Status Info for LEDL\r\n");
    }
    if(!(system_stat.flags&STAT_ACDS_VALID)){
      printf("No Status Info for ACDS\r\n");
    }
    if(!(system_stat.flags&STAT_COMM_VALID)){
      printf("No Status Info for COMM\r\n");
    }
    if(!(system_stat.flags&STAT_IMG_VALID)){
      printf("No Status Info for IMG\r\n");
    }
  }
  //send status
  ctl_events_set_clear(&cmd_parse_evt,CMD_PARSE_SEND_STAT_CMD,0);
  return 0;
}

int hard_reset_Cmd(char **argv,unsigned short argc)
{
  int i,hold=0;
  unsigned long num;
  char *end;
  char reset=0;
  if(argc==0)
  {
    printf("Error : %s requires one or more arguments\r\n",argv[0]);
    return -1;
  }
  for(i=1;i<=argc;i++)
  {
    if(!strcmp(argv[i],"hold"))
    {
      hold=1;
    }else if(!strcmp(argv[i],"LEDL"))
    {
      //set LEDL reset pin
      reset|=LEDL_RST_PIN;
    }else if(!strcmp(argv[i],"ACDS"))
    {
      //set LEDL reset pin
      reset|=ACDS_RST_PIN;
    }else if(!strcmp(argv[i],"COMM"))
    {
      //set LEDL reset pin
      reset|=COMM_RST_PIN;
    }else if(!strcmp(argv[i],"IMG"))
    {
      //set LEDL reset pin
      reset|=IMG_RST_PIN;
    }else if(!strcmp(argv[i],"all"))
    {
      //set all reset pins
      reset|=LEDL_RST_PIN|ACDS_RST_PIN|COMM_RST_PIN|IMG_RST_PIN;
    }else{
      //attempt to parse numeric value
      num=strtoul(argv[i],&end,0);
      //check if anything worked
      if(end==argv[i])
      {
        printf("Error : unknown argument %s\r\n",argv[i]);
        return -3;
      }
      //check for second argument
      if(argc!=2)
      {
        printf("Error : numeric commands require 2 arguments\r\n");
        return -7;
      }
      //check for suffix
      if(*end!='\0')
      {
        printf("Error : unknown suffix \"%s\" for \"%s\"\r\n",end,argv[i]);
        return -5;
      }
      //check range
      if(num>0xFF)
      {
        printf("Error : argument %lu is too large\r\n",num);
        return -6;
      }
      //add value in value
      reset|=num;
    }
  }
  //print out value
  printf("Setting pins 0x%02X\r\n",reset);
  //set pins to output
  P6OUT|=reset;
  //check if holding
  if(hold){
    printf("Reset Active press any key to stop\r\n");
    //wait for keypres
    getchar();
  }else{
    //wait 100ms for reset to happen
    ctl_timeout_wait(ctl_get_current_time()+100);
  }
  //set pins back to input
  P6OUT=0;
  //print message
  printf("Reset Complete\r\n");
  //done
  return 0;
}

int CDH_print_cmd(char **argv,unsigned short argc)
{
  if(argc>1){
    printf("Error : too many arguments\r\n");
    return 1;
  }
  if(argc==1){
    if(!strcmp("on",argv[1])){
      CDH_print=1;
    }else if(!strcmp("off",argv[1])){
      CDH_print=0;
    }else{
      printf("Error : unrecognized argument \"%s\"\r\n",argv[1]);
      return 2;
    }
  }
  printf("CDH printing is : %s\r\n",CDH_print?"on":"off");
  return 0;
}

int power_Cmd(char **argv,unsigned short argc){
  unsigned char cmd,addr;
  unsigned char buf[BUS_I2C_HDR_LEN+0+BUS_I2C_CRC_LEN],*ptr;
  int resp;
  //check number of arguments
  if(argc!=2){
    printf("Error : %s requires 2 aruments but %i given\r\n",argv[0],argc);
    return -1;
  }
  //get command
  if(!strcmp("on",argv[1])){
    cmd=CMD_SUB_ON;
  }else if(!strcmp("off",argv[1])){
    cmd=CMD_SUB_OFF;
  }else{
    printf("Error : unknown argument %s\r\n",argv[1]);
    return -2;
  }
  //get address
  addr=getI2C_addr(argv[2],0,busAddrSym);
  //check if address was found
  if(addr==0xFF){
    return -3;
  }
  //setup command memory
  ptr=BUS_cmd_init(buf,cmd);
  //send packet
  resp=BUS_cmd_tx(addr,buf,0,0);
  //check result
  if(resp!=RET_SUCCESS){
    printf("Error sending packet : %s\r\n",BUS_error_str(resp));
  }else{
    printf("Command sent successfully!!\r\n");
  }
}

int solar_Cmd(char **argv,unsigned short argc){
  unsigned int e;
  //clear old events
  ctl_events_set_clear(&term_evt,0,TERM_EVT_SEND_STAT);
  //wait for status information
  printf("waiting for new status\r\n");
  //wait for send stat event
  e=ctl_events_wait(CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR,&term_evt,TERM_EVT_SEND_STAT,CTL_TIMEOUT_DELAY,25*1024);
  //check if timeout occured
  if(!(e&TERM_EVT_SEND_STAT)){
    //print error
    printf("Error : Timeout\r\n");
    return 1;
  }
  //print data
  if(system_stat.flags&STAT_EPS_VALID){
    //print solar cell voltages for testing
    printf("Solar Cell voltages:\r\n");
    printf("\tX-voltage = %u\r\n",system_stat.EPS_stat.X_voltage);
    printf("\tY-voltage = %u\r\n",system_stat.EPS_stat.Y_voltage);
    printf("\tZ-voltage = %u\r\n",system_stat.EPS_stat.Z_voltage);
    return 0;
  }else{
    printf("Error : EPS stat is invalid\r\n");
    return -1;
  }
}

int I2CcmdName_Cmd(char **argv,unsigned short argc){
  int i;
  const char *name;
  for(i=0;i<256;i++){
    name=BUS_cmdtostr(i);
    if(strcmp("Unknown",name)){
      printf("\t%-25s = %3i = 0x%02X\r\n",name,i,i);
    }
  }
  return 0;
}

int I2C_spam_Cmd(char **argv,unsigned short argc){
  unsigned char addr;
  int resp;
  unsigned char buf[BUS_I2C_HDR_LEN+0+BUS_I2C_CRC_LEN];
  if(argc!=1){
    printf("Error : \"%\" requires one argument\r\n",argv[0]);
    return 1;
  }
  //get address
  addr=getI2C_addr(argv[1],0,busAddrSym);
  //check if address was found
  if(addr==0xFF){
    return -3;
  }
  //setup command
  BUS_cmd_init(buf,CMD_PING);
  //print message
  printf("Spamming %s, press any key to stop\r\n",argv[1]);
  //loop while no key is pressed
  while(UCA1_CheckKey()==EOF){
    resp=BUS_cmd_tx(addr,buf,0,0);
    if(resp!=RET_SUCCESS){
      printf("Error : %s\r\n",BUS_error_str(resp));
    }
  }
  return 0;
}

int stat_req_Cmd(char **argv,unsigned short argc){
  if(argc>0){
    if(!strcmp("on",argv[1])){
      stat_req=1;
    }else if(!strcmp("off",argv[1])){
      stat_req=0;
    }else{
      printf("Error : failed to parse \"%s\"\r\n",argv[1]);
      return -1;
    }
  }
  printf("stat_req = %s\r\n",stat_req?"on":"off");
  return 0;
}

//table of commands with help
const CMD_SPEC cmd_tbl[]={{"help"," [command]",helpCmd},
                    ARC_COMMANDS,
                    //MMC_COMMANDS,
                    //MMC_INIT_CHECK_COMMAND,MMC_DUMP_COMMAND,MMC_DAT_COMMAND,MMC_CARD_SIZE_COMMAND,MMC_ERASE_COMMAND,MMC_INIT_COMMAND,
                    ERROR_COMMANDS,
                    ARC_ASYNC_PROXY_COMMAND,
                    //CTL_COMMANDS,
                    {"stat","\r\n\t""Get status from all subsystems.", statCmd},
                    {"hreset","\r\n\t""hard reset a given system",hard_reset_Cmd},
                    {"cdhp","[on|off]...""\r\n\t""turn on or off printing",CDH_print_cmd},
                    {"power","on|off addr""\r\n\t""Power on or off a subsystem",power_Cmd},
                    {"solar","""\r\n\t""Print solar cell voltages from status",solar_Cmd},
                    {"cmd","""\r\n\t""Print command names and values",I2CcmdName_Cmd},
                    {"spam","addr""\r\n\t""spam addr with I2C commands",I2C_spam_Cmd},
                    {"req","[on|off]""\r\n\t""Get/set status of status requests",stat_req_Cmd},
                   
                   //end of list

                   {NULL,NULL,NULL}};

