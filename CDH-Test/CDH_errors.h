#ifndef __CDH_ERRORS_H
  #define __CDH_ERRORS_H
  #include <commandLib.h>
  //error sources for BUS test program
  enum{CDH_ERR_SRC_SUBSYSTEM=ERR_SRC_CMD+1,CDH_ERR_SRC_MAG,ERR_SRC_STAT,ERR_SRC_CDH_STARTUP,ERR_SRC_CDH_UTIL,CDH_ERROR_END};

  #define CDH_ERROR_MAX             (CDH_ERROR_END-1)
  #define CDH_ERROR_MIN             (ERR_SRC_CMD)
  
  //errors for magnetometer data
  enum{MAG_ERR_BAD_PACKET_LENGTH};

  //errors for status
  enum{ERROR_TOO_MANY_ERRORS,INFO_STATUS_REQ,ERR_STAT_REQ_FAIL,ERR_STAT_REQ_FAIL_COUNT,INFO_STATUS_TX,INFO_STATUS_FLAGS,ERR_STATUS_TX};

  //startup message
  enum{INFO_STARTUP_USB_PWR};

  //CDH utility errors
  enum{INFO_TESTING_RESET};

  const char *CDH_err_decode(char buf[150], unsigned short source,int err, unsigned short argument);
    
#endif
  