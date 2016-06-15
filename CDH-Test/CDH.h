#ifndef __CDH_H
#define __CDH_H

extern CTL_EVENT_SET_t cmd_parse_evt,term_evt;

extern CMD_PARSE_DAT CDH_parse;

void cmd_parse(void *p);

//parse events from the bus for the subsystem
void sub_events(void *p);

//structure for status data from subsystems (pay attention to word boundaries
typedef struct{
  unsigned char type;		//Defines type of packet, i.e. beacon
  unsigned char CDH_addr;
  unsigned char LEDL_powerup, ACDS_powerup, COMM_powerup, IMG_powerup;
  unsigned char flags;                    //Flags to determine if subsystem data is valid
  unsigned char time0,time1,time2,time3;  //time bytes
  //data from each subsystem 30 bytes is maximum that I2C can accept!
  char LEDL_addr, LEDL_stat[18];          //Defined on wiki
  char ACDS_addr, ACDS_stat[30];          //Defined on wiki
  char COMM_addr, COMM_stat[10];          //Defined on wiki
  char IMG_addr, IMG_stat[8];             //Defined on wiki
  char EPS_addr; 
  struct {
    unsigned short Yplus_current;
    unsigned short Yminus_current;
    unsigned short Y_voltage;
    unsigned short Xplus_current;
    unsigned short Xminus_current;
    unsigned short X_voltage;
    unsigned short Zminus_current;
    unsigned short Z_voltage;
    unsigned short BattBus_Current;
    unsigned short Bus5_Current;
    unsigned short Bus3_current;
    unsigned short Batt0_current;
    unsigned short Batt1_current;
    unsigned short Batt0_Voltage;
    unsigned short Batt1_Voltage;
  } EPS_stat;
  char CRC[BUS_SPI_CRC_LEN];              //padding for CRC
}STAT_PACKET;

//flags for STAT_PACKET
enum{STAT_LEDL_VALID=1<<0,STAT_ACDS_VALID=1<<1,STAT_COMM_VALID=1<<2,STAT_IMG_VALID=1<<3,STAT_EPS_VALID=1<<4};
  
//mask to see if all statuses are valid
#define STAT_ALL_VALID  (STAT_EPS_VALID|STAT_LEDL_VALID|STAT_ACDS_VALID|STAT_COMM_VALID|STAT_IMG_VALID)

//flags for cmd_parse_evt
enum{CMD_PARSE_GET_STAT_CMD=1<<0,CMD_PARSE_SEND_STAT_CMD=1<<1,CMD_PARSE_SPI_CLEAR=1<<2,CMD_PARSE_GET_STAT=1<<3,CMD_PARSE_SEND_STAT=1<<4,CMD_PARSE_ANTENNA_DEPLOY=1<<5,CMD_PARSE_RF_ON=1<<6,CMD_PARSE_BEACON_ON=1<<7,CMD_PARSE_GS_CMD=1<<8,CMD_PARSE_ACDS_ON=1<<9,CMD_PARSE_HW_RESET=1<<10};

#define CMD_PARSE_ALL (CMD_PARSE_GET_STAT_CMD|CMD_PARSE_SEND_STAT_CMD|CMD_PARSE_SPI_CLEAR|CMD_PARSE_GET_STAT|CMD_PARSE_SEND_STAT|CMD_PARSE_ANTENNA_DEPLOY|CMD_PARSE_RF_ON|CMD_PARSE_BEACON_ON|CMD_PARSE_GS_CMD|CMD_PARSE_ACDS_ON|CMD_PARSE_HW_RESET)

//flags for term_evt
enum{TERM_EVT_SEND_STAT=BIT0};

//#define minV 200
//#define DeployAntennaTime 0x001C2000  // 30 min ticker = 30*60*1024
//#define RFONTime 0x002A3000 //45 min = 45*60*1024
//reset pins for subsystems
enum  {CDH_RST_PIN=BIT0,COMM_RST_PIN=BIT1,LEDL_RST_PIN=BIT2,ACDS_RST_PIN=BIT3,IMG_RST_PIN=BIT4};

#define ALL_RST_PIN             (COMM_RST_PIN|LEDL_RST_PIN|ACDS_RST_PIN|IMG_RST_PIN)

//#define SOLAR_THRESHOLD         (750)
//#define ANT_DEPLOY_TIME         (10ul*60ul*1024ul) //10 minutes
//#define RF_ON_TIME              (45ul*60ul*1024ul) //45 min = 45*60*1024
//#define BEACON_ON_TIME          (7ul*24ul*60ul*60ul*1024ul) //1 week
#define BURN_DELAY              (10ul*1024ul)  //10 sec

//FOR TESTING
#define minV 40
#define DeployAntennaTime 0x00001000  // 30 min ticker = 30*60*1024
#define RFONTime 0x00002000 //45 min = 45*60*1024
#define SOLAR_THRESHOLD         (750)
#define ANT_DEPLOY_TIME         (20ul*1024ul)   //20 sec
#define RF_ON_TIME              (30ul*1024ul)   //30 sec
#define BEACON_ON_TIME          (5ul*60ul*1024ul) //5 min = 5*60*1024 after RFONTime

extern short beacon_on;
extern short USB_power;

extern int CDH_print;
extern int stat_req;

extern STAT_PACKET system_stat;
extern char GS_CMD[30];


//setup CDH timer to run off 32.768kHz ACLK
void init_CDH_timer(void);
//start CDH timer in continuous mode
void start_CDH_timer(void);
//start timer A

void PrintBuffer(char *dat, unsigned int len);
void CDH_timer_setup(void);
void PrintBuffer(char *dat, unsigned int len);
short USB_power_check(void);
void burn_on(void);
void burn_off(void);

#endif
