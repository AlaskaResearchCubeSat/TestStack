#include <msp430.h>
#include <ctl.h>
#include <stdio.h>
#include <ARCbus.h>
#include <string.h>
#include <SDlib.h>
#include "LEDL.h"

LEDL_STAT ledl_stat;
EPS_STAT eps_stat;
CTL_EVENT_SET_t LEDL_evt;

int LEDL_parseCmd(unsigned char src,unsigned char cmd,unsigned char *dat,unsigned short len,unsigned char flags);

CMD_PARSE_DAT LEDL_parse={LEDL_parseCmd,CMD_PARSE_ADDR0|CMD_PARSE_GC_ADDR,BUS_PRI_NORMAL,NULL};

//TODO:FILL WITH THE RIGHT COMMANDS
int LEDL_parseCmd(unsigned char src, unsigned char cmd, unsigned char *dat, unsigned short len, unsigned char flags)
{
  int i;
  switch(cmd)
  {
    case CMD_LEDL_READ_BLOCK:
      return RET_SUCCESS;
    case CMD_LEDL_BLOW_FUSE:
      return RET_SUCCESS;
  }
}

void sub_events(void *p) __toplevel{
  unsigned int e;
  int i, resp;

  for(;;)
  {
    e=ctl_events_wait(CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR,&SUB_events,SUB_EV_ALL,CTL_TIMEOUT_NONE,0);
  }
}


//handle LEDL specific commands don't wait here.
int SUB_parseCmd(unsigned char src, unsigned char cmd, unsigned char *dat, unsigned short len)
{
  return ERR_UNKNOWN_CMD;
}

void LEDL_events(void *p) __toplevel
{
  unsigned int e;
  int i, resp; 
  unsigned char buf[BUS_I2C_HDR_LEN+sizeof(EPS_STAT)+BUS_I2C_CRC_LEN],*ptr;

  //initialize status
  ledl_stat.dat[0]=0;  //for testing

  //init_event
  ctl_events_init(&LEDL_evt,0);

  //endless loop
  for(;;)
  {
    //wait for events
    e=ctl_events_wait(CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR,&LEDL_evt,LEDL_EVT_ALL,CTL_TIMEOUT_NONE,0);

//******************* SEND LEDL STATUS TO CDH ***************************
    if(e&SUB_EV_SEND_STAT)
    {
      //send status
      puts("Sending status\r\n");
      //fill in eps telemetry data (really shouldn't do it this way!)
      ptr=BUS_cmd_init(buf,CMD_EPS_STAT);
      //fill in telemetry data
      for(i=0;i<sizeof(EPS_STAT);i++)
      {
        ptr[i]=((unsigned char*)(&eps_stat))[i];
      }
     //send command
      resp=BUS_cmd_tx(BUS_ADDR_CDH,buf,sizeof(EPS_STAT),0);
      if(resp!=RET_SUCCESS)
      {
        printf("Failed to send EPS status %s\r\n",BUS_error_str(resp));
      }
      //setup packet 
      ptr=BUS_cmd_init(buf,CMD_LEDL_STAT);
      //fill in fake telemetry data
      for(i=0;i<sizeof(LEDL_STAT);i++)
      {
        ptr[i]=i;
      }
      //send command
      resp=BUS_cmd_tx(BUS_ADDR_CDH,buf,sizeof(LEDL_STAT),0);
      if(resp!=RET_SUCCESS)
      {
        printf("Failed to send LEDL status %s\r\n",BUS_error_str(resp));
      }
    }
  }
}

