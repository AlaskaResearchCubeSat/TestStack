#include <msp430.h>
#include <string.h>
#include "sensor.h"
#include "Adafruit_VC0706.h"
#include <UCA1_uart.h>
#include "IMG.h"
#include "IMG_errors.h"
#include <Error.h>
#include <SDlib.h>
#include <crc.h>
#include "status.h"

//Size in bytes of block to read from sensor
#define SENSOR_READ_BLOCK_SIZE          (64)

void sensor_init(void){

    //setup system specific peripherals
    Adafruit_VC0706_init();

    //setup P7.0 for imager on/off
    // Set imager to off to start with
    P7OUT&=~BIT0;
    //set pin direction to output
    P7DIR|=BIT0;
    //turn off pull resistor
    P7REN&=~BIT0;
    //set pin as GPIO
    P7SEL0&=~BIT0;
}

void sensor_on(void){
    //power on sensor
    P7OUT|=BIT0;
    // Let the camera boot up for a little bit...
    ctl_timeout_wait(ctl_get_current_time()+500);
    //turn power LED on
    LED_on(IMG_PWR_LED);
}

void sensor_off(void){
    //power off sensor
    P7OUT&=~BIT0;
    //turn power LED off
    LED_off(IMG_PWR_LED);
}

int savepic(void){
    uint32_t jpglen,i;
    IMG_DAT *block;
    int j;
    unsigned long baseAddr;
    int blockIdx;
    unsigned char Num;
    unsigned char* buffer=NULL;
    int resp;
    ticker pictTime;
    int bytesToRead,blockspace,bytesToWrite;
    
    //generate info message 
    report_error(ERR_LEV_DEBUG,ERR_IMG,INFO_IMG_TAKE_PIC,writePic);
    
    stat_pic_start();
    
    //set image size
    Adafruit_VC0706_setImageSize(VC0706_640x480);
    //take the picture
    if(!Adafruit_VC0706_takePicture()){
        //take picture failed, report error
        report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_TAKEPIC, 0);
        //return error
        return IMG_RET_ERR_TAKEPIC;
    }
    //capture the time that the picture was taken
    pictTime=get_ticker_time();
    
    //get frame length
    jpglen = Adafruit_VC0706_frameLength();
    //check if there is an image available
    if(jpglen == 0){
        //no image in buffer, report error
        report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_PICSIZE, 0);
        //clear in progress flag
        stat_pic_abort();
        //return error
        return IMG_RET_ERR_PICSIZE;
    }
    
    //get buffer
    block = BUS_get_buffer(CTL_TIMEOUT_DELAY,1000);
    //check if timeout expired
    if(block==NULL){
        //buffer is locked, report error
        report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_BUFFER_BUSY, 0);
        //clear in progress flag
        stat_pic_abort();
        //return error
        return IMG_RET_ERR_BUFFER_BUSY;
    }
    
    // Set block address
    baseAddr = IMG_ADDR_START + writePic * IMG_SLOT_SIZE;
    //advance picture slot
    writePic++;
    //check for wrap around
    if(writePic>=NUM_IMG_SLOTS){
        //wrap around to zero
        writePic=0;
    }
    //advance picture number
    Num=picNum++;
    
    for(i=0,blockIdx=0;i<jpglen;blockIdx++){
        //check if this is the first block
        if(i==0){
            //add time to the beginning of the first block
            memcpy(block->dat,&pictTime,sizeof(pictTime));
            j=sizeof(pictTime);
        }else{//There are never unwritten bytes for the first block
            //check for unwritten bytes
            if(bytesToRead>bytesToWrite){
                //copy into buffer
                memcpy(block->dat, buffer+bytesToWrite, bytesToRead-bytesToWrite);
                //add bytes to image index
                i+=bytesToRead-bytesToWrite;
                //set block index
                j=bytesToRead-bytesToWrite;
            }else{
                j=0;
            }
        }
        for(;j<sizeof(block->dat) && i<jpglen;){
            //check if there is more than SENSOR_READ_BLOCK_SIZE bytes to read
            if (jpglen-i > SENSOR_READ_BLOCK_SIZE){
                //read 64 bytes
                bytesToRead = SENSOR_READ_BLOCK_SIZE;
            }else{
                //calculate number of bytes remaining
                bytesToRead =jpglen-i;
            }
            //get data from sensor
            buffer = Adafruit_VC0706_readPicture(i,bytesToRead);
            //check for errors
            if(buffer==NULL){
                //error reading image data, report error
                report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_READPIC,0);
                //free buffer
                BUS_free_buffer();
                //clear in progress flag
                stat_pic_abort();
                //return error
                return IMG_RET_ERR_READ_PIC_DAT;
            }
            //number of bytes to write
            bytesToWrite=bytesToRead;
            //calculate the number of bytes left in the block
            blockspace=sizeof(block->dat)-(j);
            //check available space in block
            if(bytesToWrite>blockspace){
                //only write enough to fill the block
                bytesToWrite=blockspace;
            }
            //copy into buffer
            memcpy(&(block->dat[j]), buffer, bytesToWrite);
            //add bytes to indexes
            j+=bytesToWrite;
            i+=bytesToWrite;
        }
        //write block fields
        //set block type
        block->magic=(blockIdx==0)?BT_IMG_START:BT_IMG_BODY;
        //set image number
        block->num=Num;
        //set block number
        block->block=(blockIdx==0)?(jpglen+sizeof(block->dat)-1)/sizeof(block->dat):blockIdx;
        //calculate CRC
        block->CRC=crc16(block,sizeof(*block)-sizeof(block->CRC));
        //write block to SD card
        resp = mmcWriteBlock(baseAddr+blockIdx,(unsigned char*)block);
        //check for errors
        if(resp != MMC_SUCCESS){
            //write failed, report error
            report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_SD_CARD_WRITE,resp);
            //free buffer
            BUS_free_buffer();
            //clear in progress flag
            stat_pic_abort();
            //error encountered, abort image store
            return ERR_IMG_SD_CARD_WRITE;
        }
        //toggle LED for block write
        LED_toggle(IMG_RW_LED);
    }
    //free buffer
    BUS_free_buffer();
    //update status with picture number 
    stat_pic_complete(Num);
    //SUCCESS!!
    return IMG_RET_SUCCESS;
}


int loadpic(void){
    int writeCount = 0;
    int count = 0;
    int nextBlock = 0;
    int i,j;
    int img_size;
    int resp,found;
    unsigned short check;
    unsigned long imgStart;
    IMG_DAT *block;
    unsigned char *buffer=NULL;
    
    //reserve buffer
    buffer=BUS_get_buffer(CTL_TIMEOUT_DELAY,1000);
    //check for errors
    if(buffer==NULL){
        //report error
        report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_LOADPIC_BUFFER,0);
        //return error
        return IMG_RET_ERR_BUFFER_BUSY;
    }
    //address for imager data
    block=(IMG_DAT*)(buffer+2);
    //calculate address
    imgStart=IMG_ADDR_START+readPic*IMG_SLOT_SIZE; 
    //read from SD card
    resp=mmcReadBlock(imgStart+readBlock,(unsigned char*)block);
    //check for errors
    if(resp != MMC_SUCCESS){
        //report error
        report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_SD_CARD_READ, resp);
        //done with buffer, free it
        BUS_free_buffer();
        //return error
        return IMG_RET_ERR_SD_READ;
    }
    //check if block type is correct
    if(block->magic!=BT_IMG_BODY){
        //report error
        report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_READ_BLOCK_ID,block->magic);
        //done with buffer, free it
        BUS_free_buffer();
        //return error
        return IMG_RET_BAD_BLK_HEADER;
    }
    //calculate CRC
    check=crc16(block,sizeof(*block)-sizeof(block->CRC));
    //check if CRC's match
    if(check!=block->CRC){
        //bad CRC, report error
        report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_READ_INVALID_CRC,block->CRC);
        //done with buffer, free it
        BUS_free_buffer();
        //return error
        return IMG_RET_BAD_CRC;
    }
    //check block number
    if(readBlock!=block->block){
        //report error
        report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_READ_BLOCK_NUM_MISMATCH,((unsigned short)block->block)|(readBlock<<8));
        //done with buffer, free it
        BUS_free_buffer();
        //return error
        return IMG_RET_READ_BLOCK_NUM_MISMATCH;
    }        
    //The correct block was found, transmit
    //set block type
    buffer[0]=SPI_IMG_DAT;
    //set source address
    buffer[1]=BUS_get_OA();
    // Transmit this block across SPI
    resp = BUS_SPI_txrx(BUS_ADDR_COMM,buffer,NULL,sizeof(IMG_DAT) + 2);
    //check for errors
    if(resp != RET_SUCCESS){
        //report error
        report_error(ERR_LEV_ERROR,ERR_IMG,ERR_IMG_TX, resp);
        //done with buffer, free it
        BUS_free_buffer();
        //return error
        return IMG_RET_ERR_IMG_TX;
    }
    //Toggle read LED
    LED_toggle(IMG_READ_LED);
    //done with buffer, free it
    BUS_free_buffer();
    //SUCCESS!!
    return IMG_RET_SUCCESS;
}
