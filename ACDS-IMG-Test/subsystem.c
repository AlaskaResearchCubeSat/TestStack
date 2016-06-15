#include <msp430.h>
#include <ctl.h>
#include <stdio.h>
#include <ARCbus.h>
#include <string.h>
#include <SDlib.h>
#include "IMG.h"
#include "ACDS.h"

void sub_events(void *p) __toplevel{
  unsigned int e;
  int i;

  for(;;){
    e=ctl_events_wait(CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR,&SUB_events,SUB_EV_ALL,CTL_TIMEOUT_NONE,0);

//****************************** COMMAND TO POWER OFF ******************************
    if(e&SUB_EV_PWR_OFF){
      //print message
      puts("System Powering Down\r\n");
    }

//****************************** COMMAND TO POWER ON ******************************
    if(e&SUB_EV_PWR_ON){
      //print message
      puts("System Powering Up\r\n");
    }

//**************************** Set Status Events ****************************
    if(e&SUB_EV_SEND_STAT){
      //set events
      ctl_events_set_clear(&IMG_evt,IMG_EV_SEND_STAT,0);
      ctl_events_set_clear(&ACDS_evt,ACDS_EV_SEND_STAT,0);
    }


//**************************** RECEIVING DATA OVER SPI ****************************
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

    if(e&SUB_EV_SPI_ERR_BUSY){
      puts("SPI bad CRC\r");
    }
  }
}
