#ifndef __IMG_ERRORS_H
  #define __IMG_ERRORS_H
  #include <commandLib.h>
  //error sources for BUS test program
  enum{IMG_ERR_SRC_SUBSYSTEM=ERR_SRC_CMD+1};
  
  //error decode function
  const char *IMG_err_decode(char buf[150], unsigned short source,int err, unsigned short argument);
#endif
  