#ifndef __ACDS_H
#define __ACDS_H

  //events in COMM_evt
  enum{ACDS_EVT_STATUS_REQ=1<<3};

  #define ACDS_EVT_ALL (ACDS_EVT_STATUS_REQ)

  //structure for status data from COMM
  //TODO: figure out COMM status
  typedef struct{
    short mag[3];
    unsigned char mode;
    unsigned char tqstat[3];
    unsigned short flips[3];
    short attitude[4];
    short rates[3];
  }ACDS_STAT;

  extern ACDS_STAT status;

  //flags for STAT_PACKET

  //parse events from the bus for the subsystem
  void sub_events(void *p);

  //events for COMM task
  extern CTL_EVENT_SET_t ACDS_evt;

  //parse COMM specific events
  void ACDS_events(void *p);

  void PrintBuffer(char *dat, unsigned int len);


#endif
