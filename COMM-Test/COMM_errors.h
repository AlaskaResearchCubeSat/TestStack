#ifndef __COMM_ERRORS_H
  #define __COMM_ERRORS_H
  #include <commandLib.h>
  //error sources for BUS test program
  enum{COMM_ERR_SRC_SUBSYSTEM=ERR_SRC_CMD+1};
  
  //error sources for subsystem
  enum{COMM_ERR_SPI_CRC,COMM_ERR_SPI_BUSY};

  #define COMM_ERR_MIN      ERR_SRC_CMD
  #define COMM_ERR_MAX      COMM_ERR_SRC_SUBSYSTEM

  const char *COMM_err_decode(char buf[150], unsigned short source,int err, unsigned short argument);

#endif
  