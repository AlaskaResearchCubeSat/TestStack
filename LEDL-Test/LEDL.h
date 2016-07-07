#ifndef __LEDL_H
#define __LEDL_H

  //events in LEDL_evt
  //enum{LEDL_EVT_STATUS_REQ=1<<3};
  enum{LEDL_EV_SEND_STAT=1<<3};
  #define LEDL_EVT_ALL (LEDL_EV_SEND_STAT)
  
  //#define LEDL_EVT_ALL (LEDL_EVT_STATUS_REQ)

  //structure for status data from LEDL
  //TODO: figure out LEDL status
  typedef struct{
    unsigned char dat[18];          //Other stuff   
  }LEDL_STAT;

  typedef struct{
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
  }EPS_STAT;

  extern LEDL_STAT ledl_stat;
  extern EPS_STAT eps_stat;

  extern CMD_PARSE_DAT LEDL_parse;
  //flags for STAT_PACKET

  //parse events from the bus for the subsystem
  void sub_events(void *p);

  //events for LEDL task
  extern CTL_EVENT_SET_t LEDL_evt;

  //parse LEDL specific events
  void LEDL_events(void *p);

  void PrintBuffer(char *dat, unsigned int len);


#endif
