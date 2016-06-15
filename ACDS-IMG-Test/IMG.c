#include <msp430.h>
#include <ctl.h>
#include <stdio.h>
#include <ARCbus.h>
#include <string.h>
#include <SDlib.h>
#include "IMG.h"

static IMG_STAT status;
CTL_EVENT_SET_t IMG_evt;

int readPic,writePic;
unsigned char picNum,readBlock;

int IMG_parse_cmd(unsigned char src,unsigned char cmd,unsigned char *dat,unsigned short len,unsigned char flags);

CMD_PARSE_DAT IMG_parse={IMG_parse_cmd,CMD_PARSE_ADDR1,BUS_PRI_NORMAL,NULL};


void IMG_events(void *p) __toplevel{
  unsigned int e;
  int i, resp;
  unsigned char buf[BUS_I2C_HDR_LEN+sizeof(IMG_STAT)+BUS_I2C_CRC_LEN],*ptr;

  //set address for this task
  BUS_set_OA(BUS_ADDR_IMG);

  //initialize status

    status.dat[0]=0;           //THIS IS FOR TESTING ONLY


  //init_event
  ctl_events_init(&IMG_evt,0);

  //endless loop
  for(;;){
    //wait for events
    e=ctl_events_wait(CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR,&IMG_evt,IMG_EVT_ALL,CTL_TIMEOUT_NONE,0);
    
    //=====================[Send Status to CDH]=====================
    if(e&IMG_EV_SEND_STAT){
      //send status
      puts("Sending IMG status\r\n");
      //setup packet 
      ptr=BUS_cmd_init(buf,CMD_IMG_STAT);
      //fill in telemetry data
      for(i=0;i<sizeof(IMG_STAT);i++){
        //ptr[i]=((unsigned char*)(&status))[i];
        ptr[i]=i;
      }
      //send command
      resp=BUS_cmd_tx(BUS_ADDR_CDH,buf,sizeof(IMG_STAT),0,BUS_I2C_SEND_FOREGROUND);
      if(resp!=RET_SUCCESS){
        printf("Failed to send IMG status %s\r\n",BUS_error_str(resp));
      }
    }
    if(e&IMG_EV_TAKEPIC){
      printf("Taking Picture\r\n");
    }
    if(e&IMG_EV_LOADPIC){
      printf("IMG Read request for picture %u block %u\r\n",readPic,readBlock);
    }
  }

}

int IMG_parse_cmd(unsigned char src,unsigned char cmd,unsigned char *dat,unsigned short len,unsigned char flags){
  int i;
  int result = 0;
  ticker time;
  switch(cmd){
    //Take a picture at a specific time
    case CMD_IMG_TAKE_TIMED_PIC:
      //check packet length
      if(len!=4){
        //packet length is incorrect
        return ERR_PK_LEN;
      }
      //read time
      time =dat[3];
      time|=((ticker)dat[2])<<8;
      time|=((ticker)dat[1])<<16;
      time|=((ticker)dat[0])<<24;
      //set alarm
      BUS_set_alarm(BUS_ALARM_0,time,&IMG_evt,IMG_EV_TAKEPIC);
      return RET_SUCCESS;
    //Take a picture now
    case CMD_IMG_TAKE_PIC_NOW:
      //check packet length
      if(len!=0){
      //packet length is incorrect
      return ERR_PK_LEN;
      }
      // Call the take picture event
      ctl_events_set_clear(&IMG_evt,IMG_EV_TAKEPIC,0);
      //Return Success
      return RET_SUCCESS;
    case CMD_IMG_READ_PIC:
      //check packet length
      if(len!=2){
      //packet length is incorrect
      return ERR_PK_LEN;
      }
      // Get the picture to read
      readPic = dat[0];
      // Get the block to read
      readBlock = dat[1];
      // Call the load picture event
      ctl_events_set_clear(&IMG_evt,IMG_EV_LOADPIC,0);

      //Return Success
      return RET_SUCCESS;
    //forget that we are suposed to take a picture
    case CMD_IMG_CLEARPIC:
      if(len!=0){
        return ERR_PK_LEN;
      }
      //free the picture alarm
      BUS_free_alarm(BUS_ALARM_0);
      return RET_SUCCESS;
  }
  //Return Error
  return ERR_UNKNOWN_CMD;
}
