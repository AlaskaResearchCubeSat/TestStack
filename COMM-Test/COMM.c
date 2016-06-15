#include <msp430.h>
#include <ctl.h>
#include <stdio.h>
#include <ARCbus.h>
#include <string.h>
#include <SDlib.h>
#include <Error.h>
#include "COMM.h"
#include "COMM_errors.h"

COMM_STAT status;
CTL_EVENT_SET_t COMM_evt;
short beacon_on=0, beacon_flag=0;

int COMM_parseCmd(unsigned char src,unsigned char cmd,unsigned char *dat,unsigned short len,unsigned char flags);

CMD_PARSE_DAT COMM_parse={COMM_parseCmd,CMD_PARSE_ADDR0|CMD_PARSE_GC_ADDR,BUS_PRI_NORMAL,NULL};


//handle COMM specific commands don't wait here.
int COMM_parseCmd(unsigned char src, unsigned char cmd, unsigned char *dat, unsigned short len,unsigned char flags){
  int i;
  
  switch(cmd){
    case CMD_BEACON_ON_OFF:
     //check length
     if(len!=1){
      //error, bad length
      return ERR_PK_LEN;
     }
     //TODO: perhaps use different values and error checking
     beacon_on = dat[0];
     ctl_events_set_clear(&COMM_evt,COMM_EVT_BEACON_ON_OFF,0);
     return RET_SUCCESS;
    case CMD_BEACON_TYPE:
     //check length
     if(len!=1){
      //error, bad length
      return ERR_PK_LEN;
     }
     //TODO: perhaps use different values and error checking
     beacon_flag = dat[0];
     ctl_events_set_clear(&COMM_evt,COMM_EVT_BEACON_TYPE,0);
     return RET_SUCCESS;
   case CMD_HW_RESET:
     //check length
     if(len!=1){
      //error, bad length
      return ERR_PK_LEN;
     }
     //check address
     if(dat[0]!=BUS_ADDR_CDH){
      //error COMM can only reset CDH
      return ERR_PK_BAD_PARM;
     }
     ctl_events_set_clear(&COMM_evt,COMM_EVT_CDH_RESET,0);
     return RET_SUCCESS;
  }
  return ERR_UNKNOWN_CMD;
}

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
  unsigned short len;
  unsigned char buf[BUS_I2C_HDR_LEN+sizeof(COMM_STAT)+BUS_I2C_CRC_LEN],*ptr;

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

//******************* SEND COMM STATUS TO CDH ***************************
    if(e&SUB_EV_SEND_STAT){
      //send status
      //puts("Sending status\r\n");
      //shift LED's
      LED_shift();
      //setup packet 
      ptr=BUS_cmd_init(buf,CMD_COMM_STAT);
      //fill in telemetry data
      for(i=0;i<sizeof(COMM_STAT);i++){
        //ptr[i]=((unsigned char*)(&status))[i];
        ptr[i]=i;
      }
      //send command
      resp=BUS_cmd_tx(BUS_ADDR_CDH,buf,sizeof(COMM_STAT),0);
      ctl_events_set_clear(&COMM_evt,COMM_EVT_STATUS_REQ,0);
      if(resp!=RET_SUCCESS){
        printf("Failed to send status %s\r\n",BUS_error_str(resp));
      }
    }


// ******************* RECEIVING DATA OVER SPI *************************
    if(e&SUB_EV_SPI_DAT){
      len=arcBus_stat.spi_stat.len;
      /*printf("SPI data %u bytes recived:\r\n",len);
      //First byte contains sender address
      //Second byte contains data type
      for(i=0;i<len;i++){
        printf("0x%02X%s",arcBus_stat.spi_stat.rx[i],((i%16)==15)?"\r\n":" ");
        //TESTING: zero data
        arcBus_stat.spi_stat.rx[i]=0;
      }*/
      //free buffer
      BUS_free_buffer_from_event();
    }

    if(e&SUB_EV_SPI_ERR_CRC){
      puts("SPI bad CRC\r");
      len=arcBus_stat.spi_stat.len;
      printf("Stated transaction size : %u bytes\r\n",len);
      //print data
      for(i=0;i<len+2;i++){
        printf("0x%02X%s",arcBus_stat.spi_stat.rx[i],((i%16)==15)?"\r\n":" ");
        //TESTING: zero data
        arcBus_stat.spi_stat.rx[i]=0;
      }
      report_error(ERR_LEV_ERROR,COMM_ERR_SRC_SUBSYSTEM,COMM_ERR_SPI_CRC,0);
    }
    if(e&SUB_EV_SPI_ERR_BUSY){
      puts("SPI BUSY\r");
      report_error(ERR_LEV_ERROR,COMM_ERR_SRC_SUBSYSTEM,COMM_ERR_SPI_BUSY,0);
    }
  }
}


//handle COMM specific commands don't wait here.
int SUB_parseCmd(unsigned char src, unsigned char cmd, unsigned char *dat, unsigned short len){
  return ERR_UNKNOWN_CMD;
}

void COMM_events(void *p) __toplevel{
  unsigned int e;
  int i; 
//  char TestPacket[45];  //THIS IS FOR TESTING ONLY
//  int TestPacket_Len;   //THIS IS FOR TESTING ONLY

  //initialize status

    status.dat[0]=0;           //THIS IS FOR TESTING ONLY


  //init_event
  ctl_events_init(&COMM_evt,0);

  //endless loop
  for(;;){
    //wait for events
    e=ctl_events_wait(CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR,&COMM_evt,COMM_EVT_ALL,CTL_TIMEOUT_NONE,0);

    //update status
    if(e&COMM_EVT_STATUS_REQ){
       status.dat[0]++;                  //THIS IS FOR TESTING ONLY
                                  //Eventually need to do some actual stuff here
    }
    if(e&COMM_EVT_BEACON_ON_OFF){
      printf("Beacon = %s\r\n",beacon_on?"on":"off");
    }
    if(e&COMM_EVT_BEACON_TYPE){
      printf("Beacon = %s\r\n",beacon_flag?"status":"hello");
    }
    if(e&COMM_EVT_CDH_RESET){
      //print out value
      printf("Resetting CDH\r\n");
      //reset pin high
      P6OUT|=BIT0;
      //wait 100ms for reset to happen
      ctl_timeout_wait(ctl_get_current_time()+100);
      //reset pin low
      P6OUT&=~BIT0;
      //print message
      printf("Reset Complete\r\n");
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

