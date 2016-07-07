#include <msp430.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctl.h>
#include <terminal.h>
#include <ARCbus.h>
#include <UCA1_uart.h>
#include "SDlib.h"
#include <crc.h>
#include <commandLib.h>
#include "IMG.h"
#include <Error.h>
#include "IMG_errors.h"
#include "Adafruit_VC0706.h"
#include "sensor.h"
#include "status.h"

int cmdPic = 0;

int VidOffCmd(char **argv, unsigned short argc)
{
    printf("Video off sucessfully\r\n");
    return 0;
}

int VidOnCmd(char **argv, unsigned short argc)
{

    printf("Video on sucessfully\r\n");
    return 0;

}


int IMGCmd(char **argv, unsigned short argc)
{
  printf("You are in IMG\r\n\t");
  return 0;
}

//table of commands with help
const CMD_SPEC cmd_tbl[]={
                   {"IMG", "\r\n\t" , IMGCmd},
                   {"help"," [command]",helpCmd},
                   {"VidOff", "\r\n\t""Turns video footage off", VidOffCmd}, //testing only
                   {"VidOn", "\r\n\t" "Turns video footage on", VidOnCmd}, //testing only   
                   //end of list

                   {NULL,NULL,NULL}};

