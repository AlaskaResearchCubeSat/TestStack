#include <msp430.h>
#include <ctl.h>
#include <stdio.h>
#include <ARCbus.h>
#include <string.h>
#include <SDlib.h>
#include "IMG.h"

IMG_STAT status;
CTL_EVENT_SET_t IMG_evt;

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
  unsigned char buf[BUS_I2C_HDR_LEN+sizeof(IMG_STAT)+BUS_I2C_CRC_LEN],*ptr;

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

//******************* SEND IMG STATUS TO CDH ***************************
    if(e&SUB_EV_SEND_STAT){
      //send status
      puts("Sending status\r\n");
      //shift LED's
      LED_shift();
      //setup packet 
      ptr=BUS_cmd_init(buf,CMD_IMG_STAT);
      //fill in telemetry data
      for(i=0;i<sizeof(IMG_STAT);i++){
        //ptr[i]=((unsigned char*)(&status))[i];
        ptr[i]=i;
      }
      //send command
      resp=BUS_cmd_tx(BUS_ADDR_CDH,buf,sizeof(IMG_STAT),0);
      ctl_events_set_clear(&IMG_evt,IMG_EVT_STATUS_REQ,0);
      if(resp!=RET_SUCCESS){
        printf("Failed to send status %s\r\n",BUS_error_str(resp));
      }
    }


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


//handle COMM specific commands don't wait here.
int SUB_parseCmd(unsigned char src, unsigned char cmd, unsigned char *dat, unsigned short len){
  return ERR_UNKNOWN_CMD;
}

void IMG_events(void *p) __toplevel{
  unsigned int e;
  int i; 
//  char TestPacket[45];  //THIS IS FOR TESTING ONLY
//  int TestPacket_Len;   //THIS IS FOR TESTING ONLY

  //initialize status

    status.dat[0]=0;           //THIS IS FOR TESTING ONLY


  //init_event
  ctl_events_init(&IMG_evt,0);

  //endless loop
  for(;;){
    //wait for events
    e=ctl_events_wait(CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR,&IMG_evt,IMG_EVT_ALL,CTL_TIMEOUT_NONE,0);

    //update status
    if(e&IMG_EVT_STATUS_REQ){
       status.dat[0]++;                  //THIS IS FOR TESTING ONLY
                                  //Eventually need to do some actual stuff here
    }
  }

}

void PrintBuffer(char *dat, unsigned int len){
   int i;

   for(i=0;i<len;i++){
      printf("0X%02X ",__bit_reverse_char(dat[i])); //print MSB first so it is understandable
      if((i)%15==14){
        printf("\r\n");
      }
    }
    printf("\r\n");
}

