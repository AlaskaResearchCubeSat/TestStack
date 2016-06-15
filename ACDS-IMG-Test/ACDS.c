#include <msp430.h>
#include <ctl.h>
#include <stdio.h>
#include <ARCbus.h>
#include <string.h>
#include <SDlib.h>
#include "ACDS.h"
#include "mag.h"
#include "log.h"

static ACDS_STAT status;
CTL_EVENT_SET_t ACDS_evt;

MAG_DAT magData;

int ACDS_parse_cmd(unsigned char src,unsigned char cmd,unsigned char *dat,unsigned short len,unsigned char flags);

CMD_PARSE_DAT ACDS_parse={ACDS_parse_cmd,CMD_PARSE_ADDR0|CMD_PARSE_GC_ADDR,BUS_PRI_NORMAL,NULL};

unsigned long SD_read_addr;

void ACDS_events(void *p) __toplevel{
  unsigned int e;
  int i, resp;
  unsigned char buf[BUS_I2C_HDR_LEN+sizeof(ACDS_STAT)+BUS_I2C_CRC_LEN],*ptr;

  //set address for this task
  BUS_set_OA(BUS_ADDR_ACDS);

  //initialize status
    status.mode=0;


  //init_event
  ctl_events_init(&ACDS_evt,0);

  //endless loop
  for(;;){
    //wait for events
    e=ctl_events_wait(CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR,&ACDS_evt,ACDS_EVT_ALL,CTL_TIMEOUT_NONE,0);

    if(e&ACDS_EV_SEND_STAT){
      //send status
      puts("Sending ACDS status\r\n");

      //setup packet 
      ptr=BUS_cmd_init(buf,CMD_ACDS_STAT);
      //fill in telemetry data
      for(i=0;i<sizeof(ACDS_STAT);i++){
        //ptr[i]=((unsigned char*)(&status))[i];
        ptr[i]=i;
      }
      //send command
//      ctl_timeout_wait(ctl_get_current_time()+1);
      resp=BUS_cmd_tx(BUS_ADDR_CDH,buf,sizeof(ACDS_STAT),0,BUS_I2C_SEND_FOREGROUND);
      if(resp!=RET_SUCCESS){
        printf("Failed to send ACDS status %s\r\n",BUS_error_str(resp));
      }
    }
    if(e&ACDS_EVT_DAT_REC){
      printf("MAG data recived\r\n");
    }
    if(e&ACDS_EVT_SEND_DAT){
      printf("ACDS read request for sector %lu\r\n",SD_read_addr);
    }
  }

}

int ACDS_parse_cmd(unsigned char src,unsigned char cmd,unsigned char *dat,unsigned short len,unsigned char flags){
  unsigned long block_id;
  switch(cmd){
    case CMD_MAG_DATA:
      //check packet length
      if(len!=sizeof(magData)){
        //length incorrect, report error and exit
        return ERR_PK_LEN;
      }
      memcpy(&magData,dat,sizeof(magData));
      //sensor data recieved set event
      ctl_events_set_clear(&ACDS_evt,ACDS_EVT_DAT_REC,0);
    return RET_SUCCESS;
    case CMD_ACDS_READ_BLOCK:
      if(len!=3){
        return ERR_PK_LEN;
      }
      block_id =((unsigned long)dat[0])<<16;
      block_id|=((unsigned long)dat[1])<<8;
      block_id|=((unsigned long)dat[2]);
      //check range
      if(block_id>LOG_IDX_MAX){
        //index is out of range
        return ERR_PK_BAD_PARM;
      }
      //set SD address
      SD_read_addr=LOG_ADDR_START+block_id;
      //trigger event
      ctl_events_set_clear(&ACDS_evt,ACDS_EVT_SEND_DAT,0);
    return RET_SUCCESS;
  }
  //Return Error
  return ERR_UNKNOWN_CMD;
}
