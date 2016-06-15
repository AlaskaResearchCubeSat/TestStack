#ifndef __IMG_H
#define __IMG_H

  //events in COMM_evt
  enum{IMG_EVT_STATUS_REQ=1<<3};

  #define IMG_EVT_ALL (IMG_EVT_STATUS_REQ)

  //structure for status data from COMM
  //TODO: figure out COMM status
  typedef struct{
    unsigned char dat[8];
  }IMG_STAT;

  extern IMG_STAT status;

  //flags for STAT_PACKET

  //parse events from the bus for the subsystem
  void sub_events(void *p);

  //events for COMM task
  extern CTL_EVENT_SET_t IMG_evt;

  //parse COMM specific events
  void IMG_events(void *p);

  void PrintBuffer(char *dat, unsigned int len);


#endif
