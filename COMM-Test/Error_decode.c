#include <Error.h>
#include "COMM_errors.h"

//decode errors from CDH code
const char *COMM_err_decode(char buf[150], unsigned short source,int err, unsigned short argument){
  switch(source){
    case ERR_SRC_CMD:
      switch(err){
        case CMD_ERR_RESET:
          return "Command Line : Commanded reset";
      }
    break;         
    case COMM_ERR_SRC_SUBSYSTEM:
      switch(err){
        case COMM_ERR_SPI_CRC:
          return "COMM : Bad SPI CRC";
        case COMM_ERR_SPI_BUSY:
          return "COMM : SPI busy";
      }
    break;
  }
  sprintf(buf,"source = %i, error = %i, argument = %i",source,err,argument);
  return buf;
}

