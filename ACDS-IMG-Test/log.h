#ifndef __LOG_H
#define __LOG_H
   #include <Error.h>

    //Address range for ERROR data on the SD card
    enum{LOG_ADDR_START=ERR_ADDR_END+1,LOG_ADDR_END=LOG_ADDR_START+500,SD_FIRST_FREE_ADDR};

    #define LOG_IDX_MAX     (LOG_ADDR_START-LOG_ADDR_END-1)

    enum{ACDS_LOG_ERR_UNINIT=-15};
    
    //flags for log blocks
    enum{LOG_FLAGS_FIRST=1<<0};
    
    //initial state for flags
    #define LOG_INIT_FLAGS          LOG_FLAGS_FIRST

#endif
    