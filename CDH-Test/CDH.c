#include <msp430.h>
#include <ctl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ARCbus.h>
#include <commandLib.h>
#include <Error.h>
#include <stdlib.h>
#include "CDH_errors.h"
#include "CDH.h"


CTL_EVENT_SET_t cmd_parse_evt,term_evt;

short beacon_on=0;
short USB_power=0;
int stat_req=1;
char GS_CMD[30];

#define LED_OUT           P7OUT
#define LED_SHIFT_MASK    0x1F

enum{LED_DIR_RIGHT,LED_DIR_LEFT};
int LED_dir=LED_DIR_LEFT;

STAT_PACKET system_stat;

unsigned char reset_addr;

int CDH_parseCmd(unsigned char src,unsigned char cmd,unsigned char *dat,unsigned short len,unsigned char flags);

CMD_PARSE_DAT CDH_parse={CDH_parseCmd,CMD_PARSE_ADDR0|CMD_PARSE_GC_ADDR,BUS_PRI_NORMAL,NULL};

int CDH_print=1;

void cdh_print(const char *fmt,...){
  va_list args;
  if(CDH_print){
    //initialize va_list
    va_start(args,fmt);
    //call printf
    vprintf(fmt,args);
    //cleanup va_list
    va_end(args);
  }
}

void LED_shift(void){
  unsigned char val=LED_OUT;
  //mask out non LED bits
  val&=LED_SHIFT_MASK;
  //check direction
  if(LED_dir==LED_DIR_RIGHT){
    //shift value
    val>>=1;
    //check if we went off the end
    if(!val){
      //switch dir
      LED_dir=LED_DIR_LEFT;
      //start again
      val=BIT0;
    }
  }else{
    //shift value
    val<<=1;
    //apply mask
    val&=LED_SHIFT_MASK;
    //check if we went off the end
    if(!val){
      //reverse dir
      LED_dir=LED_DIR_RIGHT;
      //start again
      val=(LED_SHIFT_MASK+1)>>1;
    }
  }
  //set LED's
  LED_OUT=(LED_OUT&(~LED_SHIFT_MASK))|val;
}


//setup CDH timer to run off 32.768kHz ACLK
void init_CDH_timer(void){
  //setup timer A0 
  TA0CTL=TASSEL_1|ID_0|TACLR|TAIE;
  //CCR1 used for LED shifting
  TA0CCR1=32768;
  TA0CCTL1=CCIE;
}

//start CDH timer in continuous mode
void start_CDH_timer(void){
//start timer A
  TA0CTL|=MC_2;
}

//================[Time Tick interrupt]=========================
void task_tick(void) __ctl_interrupt[TIMER0_A1_VECTOR]{
  static int sec=0;
  switch(TA0IV){
    case TA0IV_TACCR1:
      LED_shift();
    break;
    case TA0IV_TAIFG:
      LED_shift();
 //     ctl_events_set_clear(&cmd_parse_evt,CMD_PARSE_GET_STAT,0);
      sec+=2;
      LED_OUT^=BIT7; //toggle bit 7
//      ctl_events_set_clear(&cmd_parse_evt,CMD_PARSE_SEND_STAT,0);
      if(sec==8){
        //set LED's and shift direction to phase correctly
        LED_dir=LED_DIR_LEFT;
        LED_OUT=(LED_OUT&(~LED_SHIFT_MASK))|((LED_SHIFT_MASK+1)>>1);
        //collect status
        ctl_events_set_clear(&cmd_parse_evt,CMD_PARSE_GET_STAT,0);
        //clear status valid flags
        system_stat.flags=0;
      }
      if(sec>=10){
        LED_OUT^=BIT5; //toggle bit 5
        sec=0;
        //send status
        ctl_events_set_clear(&cmd_parse_evt,CMD_PARSE_SEND_STAT,0);
        //also notify terminal commands
        ctl_events_set_clear(&term_evt,TERM_EVT_SEND_STAT,0);
      }
    break;
  }
}

//handle subsystem specific commands //called by the main ARCbus task don't linger here
int CDH_parseCmd(unsigned char src,unsigned char cmd,unsigned char *dat,unsigned short len,unsigned char flags){
  int i;
  switch(cmd){
    case CMD_GS_DATA:				           //Ground Station Command
      if(len==(dat[1]+3)){			   //check length, min 3 bytes + data
         memcpy(&GS_CMD,dat,len);
         ctl_events_set_clear(&cmd_parse_evt,CMD_PARSE_GS_CMD,0);
	 return RET_SUCCESS;
      }else{
        return ERR_PK_LEN;						//Packet Len Error
      }
                
    case CMD_SUB_POWERUP: //Each subsystem sends this command on powerup.
      if(len==0){
        switch(src){
                case BUS_ADDR_LEDL: 
                        system_stat.LEDL_powerup++;
                        return RET_SUCCESS;
                case BUS_ADDR_ACDS: 
                        system_stat.ACDS_powerup++;
                        //set event to turn on ACDS
                        ctl_events_set_clear(&cmd_parse_evt,CMD_PARSE_ACDS_ON,0);
                        return RET_SUCCESS;
                case BUS_ADDR_COMM: 
                        system_stat.COMM_powerup++;
                        if(beacon_on){                      // If COMM cycles after deployment need to tell COMM to RF on
                          ctl_events_set_clear(&cmd_parse_evt,CMD_PARSE_RF_ON,0);
                          ctl_events_set_clear(&cmd_parse_evt,CMD_PARSE_BEACON_ON,0);
                        }
                        return RET_SUCCESS;
                case BUS_ADDR_IMG:  
                        system_stat.IMG_powerup++;
                        return RET_SUCCESS;
        }
        //TODO: return appropriate error code
        return RET_SUCCESS;
      }else{
        return ERR_PK_LEN;
      }
	  
    case CMD_SPI_CLEAR:
      //set event
      //TODO: keep track of who is using SPI
      ctl_events_set_clear(&cmd_parse_evt,CMD_PARSE_SPI_CLEAR,0);
      return RET_SUCCESS;
    
    case CMD_EPS_STAT:							//EPS Status
      if(len==sizeof(system_stat.EPS_stat)){ 	//check length
        memcpy(&system_stat.EPS_stat,dat,len); 	//copy data into structure
        system_stat.flags|=STAT_EPS_VALID;		//set flag for EPS status
        return RET_SUCCESS;
      }else{
        return ERR_PK_LEN;						//Packet Len Error
      }
    
    case CMD_LEDL_STAT:							//LEDL status
      if(len==sizeof(system_stat.LEDL_stat)){	//check length
        memcpy(system_stat.LEDL_stat,dat,len);	//copy data into structure
        system_stat.flags|=STAT_LEDL_VALID;		//set flag for LEDL status
        return RET_SUCCESS;
      }else{
        memcpy(system_stat.LEDL_stat,dat,sizeof(system_stat.LEDL_stat)); //copy anyway
        return ERR_PK_LEN;						//Packet Len Error
      }
	  
    case CMD_ACDS_STAT:							//ACDS status
      if(len==sizeof(system_stat.ACDS_stat)){	//check length
        memcpy(system_stat.ACDS_stat,dat,len);	//copy data into structure
        system_stat.flags|=STAT_ACDS_VALID;		//set flag for ACDS status
        return RET_SUCCESS;
      }else{
        return ERR_PK_LEN;						//Packet Len Error
      }
	  
    case CMD_COMM_STAT:							//COMM status
      if(len==sizeof(system_stat.COMM_stat)){	//check length
        memcpy(system_stat.COMM_stat,dat,len);	//copy data into structure
        system_stat.flags|=STAT_COMM_VALID;		//set flag for COMM status
        return RET_SUCCESS;
      }else{
        return ERR_PK_LEN;						//Packet Len Error
      }  
    case CMD_IMG_STAT:							//IMG status
      if(len==sizeof(system_stat.IMG_stat)){	//check length
        memcpy(system_stat.IMG_stat,dat,len);	//copy data into structure
        system_stat.flags|=STAT_IMG_VALID;		//set flag for IMG status
        return RET_SUCCESS;
      }else{
        return ERR_PK_LEN;						//Packet Len Error
      }
    case CMD_HW_RESET:  
     //check length
     if(len!=1){
      //error, bad length
      return ERR_PK_LEN;
     }
     //check address
     if(dat[0]==BUS_ADDR_CDH){
      //error CDH cannot reset itself
      return ERR_PK_BAD_PARM;
     }
     //set reset addr
     //TODO: conflict prevention
     reset_addr=dat[0];
     //send event for reset
     ctl_events_set_clear(&cmd_parse_evt,CMD_PARSE_HW_RESET,0);
     return RET_SUCCESS;
  }
  //Return Error
  return ERR_UNKNOWN_CMD;
}

// this is where the work happens
void cmd_parse(void *p) __toplevel{
  unsigned int e, launch=0;
  static unsigned char buff[BUS_I2C_HDR_LEN+10+BUS_I2C_CRC_LEN];
  unsigned char *ptr;
  unsigned int num;
  ticker time;
  int resp,i,error_count=0;
  system_stat.type = SPI_BEACON_DAT;
  system_stat.CDH_addr=BUS_ADDR_CDH;
  system_stat.LEDL_addr=BUS_ADDR_LEDL;
  system_stat.ACDS_addr=BUS_ADDR_ACDS;
  system_stat.COMM_addr=BUS_ADDR_COMM;
  system_stat.IMG_addr=BUS_ADDR_IMG;
  system_stat.EPS_addr=0x16;

  USB_power = USB_power_check();

  report_error(ERR_LEV_INFO,ERR_SRC_CDH_STARTUP,INFO_STARTUP_USB_PWR,USB_power);
  
  //set bus interrupt so that LEDL knows that CDH is on
  BUS_int_set(BIT0);

  //init event
  ctl_events_init(&cmd_parse_evt,0);

  //seed random number generator from timer
  srand(TA0R);

  for(;;){
    e=ctl_events_wait(CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR,&cmd_parse_evt,CMD_PARSE_ALL,CTL_TIMEOUT_NONE,0);

    if(e&CMD_PARSE_SPI_CLEAR){
      puts("SPI bus Free\r");
      //TODO: keep track of who is using SPI
    }

    if((e&CMD_PARSE_GET_STAT && stat_req) || e&CMD_PARSE_GET_STAT_CMD){
      system_stat.flags=0;						//clear status flags from old status packet
      //status request info message
      report_error(ERR_LEV_INFO,ERR_SRC_STAT,INFO_STATUS_REQ,0);
      ptr=BUS_cmd_init(buff,CMD_SUB_STAT);                              //setup packet 
      time=get_ticker_time();                                           //get time
      ptr[0]=time>>24;							//write time into the array
      ptr[1]=time>>16;
      ptr[2]=time>>8;
      ptr[3]=time;
      resp=BUS_cmd_tx(BUS_ADDR_GC,buff,4,0);	//send command
      //Check if there was an error
      if(resp!=RET_SUCCESS){
        //Status request failed error message
        report_error(ERR_LEV_ERROR,ERR_SRC_STAT,ERR_STAT_REQ_FAIL,resp);
        //count errors
        error_count++;
      }else{
        error_count=0;
      }
      if(error_count==5){
        
        report_error(ERR_LEV_ERROR,ERR_SRC_STAT,ERR_STAT_REQ_FAIL_COUNT,0);
        //send reset command
        BUS_cmd_init(buff,CMD_RESET);
        resp=BUS_cmd_tx(BUS_ADDR_GC,buff,0,0);
        //check for error
        if(resp!=RET_SUCCESS){
          cdh_print("Soft reset failed, attempting hard reset\r\n");
          //Do a hard reset of systems
          P6OUT|=ALL_RST_PIN;
          //check if holding
          //wait 100ms for reset to happen
          ctl_timeout_wait(ctl_get_current_time()+100);
          //set pins back to input
          P6OUT&=~ALL_RST_PIN;
        }
      }
      if(error_count>7){
         cdh_print("Failed to send status too many times resetting CDH\r\n");
         //reset CDH
         reset(ERR_LEV_ERROR,ERR_SRC_STAT,ERROR_TOO_MANY_ERRORS,0);
      }
      //TESTING: randomly reset a subsystem
      //get a random number between 0 and 24
      num=rand()%24;
      //check and reset subsystem
      switch(num){
        case 1:
          P6OUT|=LEDL_RST_PIN;
          //send info message about which system is being reset
          report_error(ERR_LEV_INFO,ERR_SRC_CDH_UTIL,INFO_TESTING_RESET,BUS_ADDR_LEDL);
        break;
        case 2:
          P6OUT|=ACDS_RST_PIN;
          //send info message about which system is being reset
          report_error(ERR_LEV_INFO,ERR_SRC_CDH_UTIL,INFO_TESTING_RESET,BUS_ADDR_ACDS);
        break;
        case 3:
          P6OUT|=COMM_RST_PIN;
          //send info message about which system is being reset
          report_error(ERR_LEV_INFO,ERR_SRC_CDH_UTIL,INFO_TESTING_RESET,BUS_ADDR_COMM);
        break;
        case 4:
          P6OUT|=IMG_RST_PIN;
          //send info message about which system is being reset
          report_error(ERR_LEV_INFO,ERR_SRC_CDH_UTIL,INFO_TESTING_RESET,BUS_ADDR_IMG);
        break;
        default:
         num=0xFF;
      }
      if(num!=0xFF){  
        //wait 100ms for reset to happen
        ctl_timeout_wait(ctl_get_current_time()+100);
        //stop resetting
        P6OUT=0;
      }
    }

    if((e&CMD_PARSE_SEND_STAT && stat_req) || e&CMD_PARSE_SEND_STAT_CMD){
    // if beacon_on true send data to comm
    // if beacon_on false check eps for positive power set time counter for antenna deployment and beacon_on
      if(beacon_on){							// beacon_on = true, Send data to COMM
        report_error(ERR_LEV_INFO,ERR_SRC_STAT,INFO_STATUS_TX,0);
        system_stat.type=SPI_BEACON_DAT;        // Type = SPI_BEACON_DAT
        time=get_ticker_time();					//get time for beacon
        system_stat.time0=time>>24;				//write time into status
        system_stat.time1=time>>16;
        system_stat.time2=time>>8;
        system_stat.time3=time;
												//send SPI data
        resp=BUS_SPI_txrx(BUS_ADDR_COMM,(unsigned char*)&system_stat,NULL,sizeof(STAT_PACKET)-BUS_SPI_CRC_LEN);
        if(resp!=RET_SUCCESS){
          report_error(ERR_LEV_ERROR,ERR_SRC_STAT,ERR_STATUS_TX,resp);
        }
        ptr=(unsigned char*)&system_stat;
		
		//FOR TEST ONLY - Print out status buffer
        for(i=0;i<11;i++){
          cdh_print("0x%02X ",ptr[i]);
        }
        cdh_print("\r\n");
        cdh_print("0x%02X ",system_stat.LEDL_addr);
        PrintBuffer(system_stat.LEDL_stat, sizeof(system_stat.LEDL_stat));
        cdh_print("0x%02X ",system_stat.ACDS_addr);
        PrintBuffer(system_stat.ACDS_stat, sizeof(system_stat.ACDS_stat));
        cdh_print("0x%02X ",system_stat.COMM_addr);
        PrintBuffer(system_stat.COMM_stat, sizeof(system_stat.COMM_stat));
        cdh_print("0x%02X ",system_stat.IMG_addr);
        PrintBuffer(system_stat.IMG_stat, sizeof(system_stat.IMG_stat));
        cdh_print("0x%02X ",system_stat.EPS_addr);
        PrintBuffer((char*)&system_stat.EPS_stat, sizeof(system_stat.EPS_stat));

        report_error((system_stat.flags&STAT_ALL_VALID)==STAT_ALL_VALID?ERR_LEV_INFO:ERR_LEV_ERROR,ERR_SRC_STAT,INFO_STATUS_FLAGS,system_stat.flags);

		//FOR TEST ONLY - Print out status buffer
		
      } else { // beacon_on = FALSE start_up routine
      // check eps status for positive power
        cdh_print("Launch = %d; USB_power = %d\r\n",launch,USB_power);
        cdh_print("Waiting for Solar Cell voltage above threshold: \r\n");
        cdh_print("\tSolar Cell X-voltage = %u\r\n",system_stat.EPS_stat.X_voltage);
        cdh_print("\tSolar Cell Y-voltage = %u\r\n",system_stat.EPS_stat.Y_voltage);
        cdh_print("\tSolar Cell Z-voltage = %u\r\n",system_stat.EPS_stat.Z_voltage);
        if((system_stat.flags&STAT_LEDL_VALID) &&                                             //check that LEDL status is valid
           ((system_stat.EPS_stat.X_voltage<= SOLAR_THRESHOLD) ||
           (system_stat.EPS_stat.Y_voltage<= SOLAR_THRESHOLD && !USB_power_check()) ||        //Y BCR gets USB voltage, ignore if USB is connected
           (system_stat.EPS_stat.Z_voltage<= SOLAR_THRESHOLD))){ // positive voltage detected
          cdh_print("Solar Cell voltage above threshold\r\n");
          if(!launch){ //assuming we haven't been here before start the deployment timers.
            cdh_print("Set Antenna Deployment and RF ON timers\r\n");
            BUS_set_alarm(BUS_ALARM_0,get_ticker_time()+ANT_DEPLOY_TIME,&cmd_parse_evt,CMD_PARSE_ANTENNA_DEPLOY);
            BUS_set_alarm(BUS_ALARM_1,get_ticker_time()+RF_ON_TIME,&cmd_parse_evt,CMD_PARSE_RF_ON);
            //power up the ACDS board
            ptr=BUS_cmd_init(buff,CMD_SUB_ON);
            resp=BUS_cmd_tx(BUS_ADDR_ACDS,buff,0,0);
            if(resp!=RET_SUCCESS){
              cdh_print("Failed to send POWER ON to ACDS %s\r\n",BUS_error_str(resp));
            }
            launch=1;
          }
        }
      }
    }

    if(e&CMD_PARSE_ANTENNA_DEPLOY){
      cdh_print("Deploy antenna\r");
      if(!USB_power){
           burn_on();
          //delay for specified time
          ctl_timeout_wait(ctl_get_current_time()+BURN_DELAY);
          //turn off resistor
          burn_off();
          //Deploy the antenna only if the USB_power is not on!
          //Deployment should happen 10+ minutes after launch. P6.7
      } else cdh_print("USB power do not deploy.\r\n");

    }

    if(e&CMD_PARSE_RF_ON){                          // TURN ON HELLO MESSAGE IN BEACON
      cdh_print("Turn Beacon ON\r");
      // when timer times out send on command to comm, set beacon_on = 1;
      beacon_on = 1;
      if(!USB_power){
        ptr=BUS_cmd_init(buff,CMD_SUB_ON);
        resp=BUS_cmd_tx(BUS_ADDR_COMM,buff,0,0);
        if(resp!=RET_SUCCESS){
          cdh_print("Failed to send POWER ON to COMM %s\r\n",BUS_error_str(resp));
        }
        resp=BUS_set_alarm(BUS_ALARM_1,get_ticker_time()+BEACON_ON_TIME,&cmd_parse_evt,CMD_PARSE_BEACON_ON);
        if(resp!=RET_SUCCESS){
          cdh_print("Failed to set STATUS ON Alarm %s\r\n",BUS_error_str(resp));
        }  
      }
    }

    if(e&CMD_PARSE_BEACON_ON){                        // TURN ON STATUS MESSAGE IN BEACON
       cdh_print("Turn on Status Beacon Message\r");
       ptr=BUS_cmd_init(buff,CMD_BEACON_ON_OFF);
       //turn on beacon 
       *ptr++=1;
       resp=BUS_cmd_tx(BUS_ADDR_COMM,buff,1,0);
       if(resp!=RET_SUCCESS){
          cdh_print("Failed to send BEACON ON to COMM %s\r\n",BUS_error_str(resp));
      }
    }

    if(e&CMD_PARSE_GS_CMD){ //Just pass data along to where it needs to go
      //setup packet 
      ptr=BUS_cmd_init(buff,GS_CMD[2]);
      //fill in telemetry data
      for(i=0;i<GS_CMD[1];i++){
          ptr[i]=GS_CMD[3+i];
      }
      //send command
      resp=BUS_cmd_tx(GS_CMD[0],buff,GS_CMD[1],0);
      if(resp!=RET_SUCCESS){
          cdh_print("Failed to send GS CMD to CDHs %s\r\n",BUS_error_str(resp));
      }
    }

    if(e&CMD_PARSE_ACDS_ON){   
      //power up the ACDS board
      ptr=BUS_cmd_init(buff,CMD_SUB_ON);
      resp=BUS_cmd_tx(BUS_ADDR_ACDS,buff,0,0);
      if(resp!=RET_SUCCESS){
        cdh_print("Failed to send POWER ON to ACDS %s\r\n",BUS_error_str(resp));
      }
    }
    if(e&CMD_PARSE_HW_RESET){  
      const unsigned char addrs[5]={BUS_ADDR_GC,BUS_ADDR_COMM,BUS_ADDR_LEDL,BUS_ADDR_ACDS,BUS_ADDR_IMG};
      const unsigned char masks[5]={LEDL_RST_PIN|ACDS_RST_PIN|COMM_RST_PIN|IMG_RST_PIN,
                                    COMM_RST_PIN,LEDL_RST_PIN,ACDS_RST_PIN,IMG_RST_PIN};
      unsigned char mask;
      //look for address
      for(i=0,mask=0;i<5;i++){
        //check for a match
        if(addrs[i]==reset_addr){
          //save mask
          mask=masks[i];
          //done
          break;
        }
      }
      if(mask==0){
        //TODO: report error
      }else{
        //TODO: info message
        //print out value
        cdh_print("Performing hard reset\r\n");
        //set pin(s) high
        P6OUT|=mask;
        //wait 10ms for reset to happen
        ctl_timeout_wait(ctl_get_current_time()+10);
        //set pin(s) low
        P6OUT&=~BIT0;
        //print message
        cdh_print("Reset Complete\r\n");
      }
    }
    //end event loop
  }
}

//parse events from the bus for the subsystem
void sub_events(void *p) __toplevel{
  unsigned int e,len;
  int i;
  unsigned char buf[10],*ptr;
  for(;;){
    e=ctl_events_wait(CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR,&SUB_events,SUB_EV_ALL,CTL_TIMEOUT_NONE,0);

    if(e&SUB_EV_PWR_OFF){
      //print message
      puts("System Powering Down\r");
    }

    if(e&SUB_EV_PWR_ON){
      //print message
      puts("System Powering Up\r");
    }

    if(e&SUB_EV_SPI_DAT){
      puts("SPI data recived:\r");
      //get length
      len=arcBus_stat.spi_stat.len;
      //print out data
      for(i=0;i<len;i++){
        //printf("0x%02X ",rx[i]);
        printf("%03i ",arcBus_stat.spi_stat.rx[i]);
      }
      printf("\r\n");
      //free buffer
      BUS_free_buffer_from_event();
    }

    if(e&SUB_EV_SPI_ERR_CRC){
      puts("SPI bad CRC\r");
    }

    if(e&SUB_EV_SPI_ERR_BUSY){
      puts("SPI packet rejected : Buffer Busy\r");
    }
  }
}

void PrintBuffer(char *dat, unsigned int len){
   int i;

   for(i=0;i<len;i++){
      cdh_print("0X%02X ", dat[i]);
      if((i)%15==14){
        cdh_print("\r\n");
      }
    }
    cdh_print("\r\n");
}

//return nonzero if USB power is applied
short USB_power_check(void){
  return !(P1IN & BIT1);
//  return 0; //test
}

void burn_on(void){
    //turn on LED
    P7OUT|=BIT7;
    //turn on resistor
    P6OUT|=BIT7;
}

void burn_off(void){
    //turn off resistor
    P6OUT&=~BIT7;
    //turn off LED
    P7OUT&=~BIT7;
}
