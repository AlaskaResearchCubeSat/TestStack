#ifndef __IMG_H
#define __IMG_H

  //events in IMGevt
  enum{IMG_EV_SEND_STAT=1<<0,IMG_EV_TAKEPIC=1<<1,IMG_EV_LOADPIC=1<<2};

  #define IMG_EVT_ALL (IMG_EV_SEND_STAT|IMG_EV_TAKEPIC|IMG_EV_LOADPIC)

  //structure for status data from COMM
  //TODO: figure out COMM status
  typedef struct{
    unsigned char dat[8];
  }IMG_STAT;

  //flags for STAT_PACKET

  //parse events from the bus for the subsystem
  void sub_events(void *p);

  //events for COMM task
  extern CTL_EVENT_SET_t IMG_evt;

  //command parse structure
  extern CMD_PARSE_DAT IMG_parse;

  //parse COMM specific events
  void IMG_events(void *p);

  void PrintBuffer(char *dat, unsigned int len);


#endif
