#ifndef __COMM_H
#define __COMM_H

  //events in COMM_evt
  enum{COMM_EVT_BEACON_ON_OFF=BIT0,COMM_EVT_BEACON_TYPE=BIT1,COMM_EVT_STATUS_REQ=BIT2,COMM_EVT_CDH_RESET=BIT3};

  #define COMM_EVT_ALL (COMM_EVT_STATUS_REQ|COMM_EVT_BEACON_ON_OFF|COMM_EVT_BEACON_TYPE|COMM_EVT_CDH_RESET)

  //structure for status data from COMM
  //TODO: figure out COMM status
  typedef struct{
    unsigned char dat[10];
  }COMM_STAT;

  extern COMM_STAT status;

  //flags for STAT_PACKET

  //parse events from the bus for the subsystem
  void sub_events(void *p);

  //events for COMM task
  extern CTL_EVENT_SET_t COMM_evt;

  //structure for registering command parse callback
  extern CMD_PARSE_DAT COMM_parse;

  //function for parsing COMM commands
  int COMM_parseCmd(unsigned char src,unsigned char cmd,unsigned char *dat,unsigned short len,unsigned char flags);

  //parse COMM specific events
  void COMM_events(void *p);

  void PrintBuffer(char *dat, unsigned int len);


#endif
