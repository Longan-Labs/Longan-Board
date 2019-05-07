// Longan Card

#ifndef __LONGANCARDS_H__
#define __LONGANCARDS_H__

#include <Arduino.h>
#include "CardsDfs.h"

class CARD_INFO
{
public:

    unsigned char addr;
    unsigned char deviceID;
    unsigned char senCnt;
    unsigned char nameLen;
    char name[20];
    long sku;
    float version;


public:
    
    void setInfo(unsigned char __addr,          // I2C ADDR
                 int __sensorNo,                // SENSOR NUNBER
                 int __senCnt,                  // NUMBER OF SENSOR
                 int __nameLen,                 // LENGTH OF NAME
                 char __name[20],               // NAME OF SENSOR
                 long __sku,                    // SKU
                 float __version                // VERSION
                );
    
    void getInfo();                             // get Info from I2C
    
    unsigned char scan();
    void disp();
    //float CARD_INFO::str2float(unsigned char *str);
    
    

private:

};

#endif