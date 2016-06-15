#ifndef __LEDL_ERRORS_H
  #define __LEDL_ERRORS_H
  #include <commandLib.h>
  //error sources for BUS test program
  enum{LEDL_ERR_SRC_SUBSYSTEM=ERR_SRC_CMD+1};
  
  #define LEDL_MIN_ERR            ERR_SRC_CMD
  #define LEDL_MAX_ERR            LEDL_ERR_SRC_SUBSYSTEM

  const char *LEDL_err_decode(char buf[150], unsigned short source,int err, unsigned short argument);

#endif
  