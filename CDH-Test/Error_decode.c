#include <Error.h>
#include <string.h>
#include "CDH_errors.h"
#include "CDH.h"

//decode errors from CDH code
const char *CDH_err_decode(char buf[150], unsigned short source,int err, unsigned short argument){
  switch(source){
    case ERR_SRC_CMD:
      switch(err){
        case CMD_ERR_RESET:
          return "Command Line : Commanded reset";
      }
    break;    
    case CDH_ERR_SRC_MAG:
        switch(err){
          case MAG_ERR_BAD_PACKET_LENGTH:
            sprintf(buf,"Mag : Bad packet length %i",argument);
            return buf;
        }
    break;         
    case ERR_SRC_STAT:
      switch(err){
        case ERROR_TOO_MANY_ERRORS:
          return "Status : Too many errors";
        case INFO_STATUS_REQ:
          return "Status : Requesting Status";
        case ERR_STAT_REQ_FAIL:
          sprintf(buf,"Status : Error sending status request %s (0x%02X)",BUS_error_str(argument),argument);
          return buf;
        case ERR_STAT_REQ_FAIL_COUNT:
          return "Status : Failed to send status too many times attempting soft reset";
        case INFO_STATUS_TX:
          return "Status : Sending status packet";
        case INFO_STATUS_FLAGS:
          //check flags
          if((argument&STAT_ALL_VALID)==STAT_ALL_VALID){
            return "Status : All systems reported status";
          }else{
            sprintf(buf,"Status : systems did not report status (flags = 0x%02X) : ",argument);
            if(!(argument&STAT_EPS_VALID)) {
              strcat(buf,"EPS, ");
            }
            if(!(argument&STAT_LEDL_VALID)){
              strcat(buf,"LEDL, ");
            }
            if(!(argument&STAT_ACDS_VALID)){
              strcat(buf,"ACDS, ");
            }
            if(!(argument&STAT_COMM_VALID)){
              strcat(buf,"COMM, ");
            }
            if(!(argument&STAT_IMG_VALID)){
              strcat(buf,"IMG, ");
            }
            //remove last space and comma
            buf[strlen(buf)-2]='\0';
            //return string
            return buf;
          }
        case ERR_STATUS_TX:
          sprintf(buf,"Status : failed to send beacon data : %s (%i)\r\n",BUS_error_str(argument),argument);
          return buf;
      }
    break;   
    case ERR_SRC_CDH_STARTUP:
      switch(err){
        case INFO_STARTUP_USB_PWR:
          sprintf(buf,"CDH startup: USB power %s (0x%02X)",argument?"on":"off",argument);
          return buf;
      }
    break;
    case ERR_SRC_CDH_UTIL:
      switch(err){
        case INFO_TESTING_RESET:
          sprintf(buf,"CDH util : hard resetting %s system",I2C_addr_revlookup(argument,busAddrSym));
          return buf;
      }
    break;
  }
  sprintf(buf,"source = %i, error = %i, argument = %i",source,err,argument);
  return buf;
}
