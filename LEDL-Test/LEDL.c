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

#define LED_OUT       P7OUT

enum{LED_DIR_RIGHT,LED_DIR_LEFT};
int LED_dir=LED_DIR_LEFT;

void LED_shift(void){
  if(LED_dir==LED_DIR_RIGHT){
    LED_OUT>>=1;
    if(!LED_OUT){
      LED_dir=LED_DIR_LEFT;
      LED_OUT=BIT0;
    }
  }else{
    LED_OUT<<=1;
    if(!LED_OUT){
      LED_dir=LED_DIR_RIGHT;
      LED_OUT=BIT7;
    }
  }
}

void sub_events(void *p) __toplevel{
  unsigned int e;
  int i, resp;
  unsigned char buf[BUS_I2C_HDR_LEN+sizeof(EPS_STAT)+BUS_I2C_CRC_LEN],*ptr;

  for(;;){
    e=ctl_events_wait(CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR,&SUB_events,SUB_EV_ALL,CTL_TIMEOUT_NONE,0);

//******************* COMMAND TO POWER OFF??? NOTHING HAPPENING HERE **************
    if(e&SUB_EV_PWR_OFF){
      //print message
      puts("System Powering Down\r\n");
    }

//******************* COMMAND TO POWER ON??? NOTHING HAPPENING HERE **************
    if(e&SUB_EV_PWR_ON){
      //print message
      puts("System Powering Up\r\n");
    }

//******************* SEND LEDL STATUS TO CDH ***************************
    if(e&SUB_EV_SEND_STAT){
      //send status
      puts("Sending status\r\n");
      //shift LED's
      LED_shift();
      //fill in eps telemetry data (really shouldn't do it this way!)
      ptr=BUS_cmd_init(buf,CMD_EPS_STAT);
      //fill in telemetry data
      for(i=0;i<sizeof(EPS_STAT);i++){
        ptr[i]=((unsigned char*)(&eps_stat))[i];
      }
     //send command
      resp=BUS_cmd_tx(BUS_ADDR_CDH,buf,sizeof(EPS_STAT),0);
      if(resp!=RET_SUCCESS){
        printf("Failed to send EPS status %s\r\n",BUS_error_str(resp));
      }
      //setup packet 
      ptr=BUS_cmd_init(buf,CMD_LEDL_STAT);
      /*//fill in telemetry data
      for(i=0;i<sizeof(LEDL_STAT);i++){
        ptr[i]=((unsigned char*)(&ledl_stat))[i];
      }*/
      //fill in fake telemetry data
      for(i=0;i<sizeof(LEDL_STAT);i++){
        ptr[i]=i;
      }
      //send command
      resp=BUS_cmd_tx(BUS_ADDR_CDH,buf,sizeof(LEDL_STAT),0);
      if(resp!=RET_SUCCESS){
        printf("Failed to send LEDL status %s\r\n",BUS_error_str(resp));
      }
      ctl_events_set_clear(&LEDL_evt,LEDL_EVT_STATUS_REQ,0);
    }

//**************** NOT SURE HOW THIS IS CALLED ************************


// ******************* RECEIVING DATA OVER SPI *************************
    if(e&SUB_EV_SPI_DAT){
      puts("SPI data recived:\r");
      //First byte contains sender address
      //Second byte contains data type
      //free buffer
      BUS_free_buffer_from_event();
    }

    if(e&SUB_EV_SPI_ERR_CRC){
      puts("SPI bad CRC\r");
    }
  }
}


//handle LEDL specific commands don't wait here.
int SUB_parseCmd(unsigned char src, unsigned char cmd, unsigned char *dat, unsigned short len){
  return ERR_UNKNOWN_CMD;
}

void LEDL_events(void *p) __toplevel{
  unsigned int e;
  int i; 
//  char TestPacket[45];  //THIS IS FOR TESTING ONLY
//  int TestPacket_Len;   //THIS IS FOR TESTING ONLY

  //initialize status
  ledl_stat.dat[0]=0;  //for testing

  //init_event
  ctl_events_init(&LEDL_evt,0);

  //endless loop
  for(;;){
    //wait for events
    e=ctl_events_wait(CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR,&LEDL_evt,LEDL_EVT_ALL,CTL_TIMEOUT_NONE,0);

    //update status
    if(e&LEDL_EVT_STATUS_REQ){
       ledl_stat.dat[0]++;                  //THIS IS FOR TESTING ONLY
       eps_stat.Y_voltage=eps_stat.Y_voltage+20;
    }
  }

}

void PrintBuffer(char *dat, unsigned int len){
   int i;

   for(i=0;i<len;i++){
      printf("0X%02X ", dat[i]);
      if((i)%15==14){
        printf("\r\n");
      }
    }
    printf("\r\n");
}


