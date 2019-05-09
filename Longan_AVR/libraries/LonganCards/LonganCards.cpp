// LONGAN CARDS
#include <Wire.h>
#include <EEPROM.h>

#include "LonganCards.h"
#include "cardBasic.h"

void CARD_INFO::setInfo(unsigned char __addr,   // I2C ADDR
                 int __sensorNo,                // SENSOR NUNBER
                 int __senCnt,                  // NUMBER OF SENSOR
                 int __nameLen,                 // LENGTH OF NAME
                 char __name[20],               // NAME OF SENSOR
                 long __sku,                    // SKU
                 float __version)               // VERSION
{
    addr        = __addr;
    deviceID    = __sensorNo;
    senCnt      = __senCnt;
    nameLen     = __nameLen;
    memcpy(name, __name, 20);
    sku         = __sku;
    version     = __version;
}
    
unsigned char CARD_INFO::scan()
{

    for(int i = 1; i < 127; i++ )
    {
        Wire.beginTransmission(i);
        unsigned char error = Wire.endTransmission();
 
        if (error == 0)
        {
            addr = i;
            return i;
        }  
    }
    
    return 0;   
}

void CARD_INFO::getInfo()
{
   
    //**************GET SENSOR NO.******************************************
    Wire.beginTransmission(addr); // transmit to device #8
    Wire.write(REG_DEVIC);  
    Wire.endTransmission();    // stop transmitting
    delay(10);
    
    Wire.requestFrom((uint8_t)addr, (uint8_t)1);    // request 6 bytes from slave device #8
    delay(10);
    
    while (Wire.available())
    { // slave may send less than requested
        deviceID = Wire.read();
    }
    delay(10);
    
    //**************GET SENSOR CNT******************************************
    Wire.beginTransmission(addr); // transmit to device #8
    Wire.write(REG_SENSOR_CNT);  
    Wire.endTransmission();    // stop transmitting
    delay(10);
    
    Wire.requestFrom((uint8_t)addr, (uint8_t)1);    // request 6 bytes from slave device #8
    delay(10);
    
    while (Wire.available())
    { // slave may send less than requested
        senCnt = Wire.read();
    }
    delay(10);
    
    //**************GET NAME LENGTH******************************************
    Wire.beginTransmission(addr); // transmit to device #8
    Wire.write(REG_NAME_LEN);  
    Wire.endTransmission();    // stop transmitting
    delay(10);
    
    Wire.requestFrom((uint8_t)addr, (uint8_t)1);    
    delay(10);
    
    while (Wire.available())
    { // slave may send less than requested
        nameLen = Wire.read();
    }
    delay(10);
    
    //**************GET NAME******************************************
    Wire.beginTransmission(addr); // transmit to device #8
    Wire.write(REG_NAME);  
    Wire.endTransmission();    // stop transmitting
    delay(10);
    
    Wire.requestFrom((uint8_t)addr, (uint8_t)nameLen);    // request 6 bytes from slave device #8

    if (Wire.available())
    { // slave may send less than requested

        for(int i=0; i<nameLen; i++)
        {
            name[i] = Wire.read();
        }

        name[nameLen] = '\0';
    }
    
    delay(10);
    
    //**************GET SKU******************************************
    Wire.beginTransmission(addr); // transmit to device #8
    Wire.write(REG_SKU);  
    Wire.endTransmission();    // stop transmitting
    delay(10);
    
    Wire.requestFrom((uint8_t)addr, (uint8_t)4);    // request 6 bytes from slave device #8
    delay(10);
    
    unsigned char tmp[4];
    unsigned char tmplen = 0;
    
    while (Wire.available())
    { // slave may send less than requested
        
        tmp[tmplen++] = Wire.read();
    }
    
    if(tmplen == 4)
    {
        sku = str2float(tmp);
    }
    else
    {
        sku = 0;
    }
    delay(10);
    
    
    //**************GET VERSION******************************************
    Wire.beginTransmission(addr); // transmit to device #8
    Wire.write(REG_VERISON);  
    Wire.endTransmission();    // stop transmitting
    delay(10);
    
    Wire.requestFrom((uint8_t)addr, (uint8_t)4);    // request 6 bytes from slave device #8
    delay(10);
    
    tmplen = 0;
    while (Wire.available())
    { // slave may send less than requested
        
        tmp[tmplen++] = Wire.read();
    }
    
    if(tmplen == 4)
    {
        version = str2float(tmp);
    }
    else
    {
        version = 0;
    }
    delay(10);
}

void CARD_INFO::disp()
{
    Serial.print("ADDR: 0x");
    if(addr < 0x10)Serial.print('0');
    Serial.println(addr, HEX);
    
    Serial.print("DEVICE ID.");
    Serial.println(deviceID);
    
    Serial.print("SENSOR CNT: ");
    Serial.println(senCnt);
    
    Serial.print("NAME LEN: ");
    Serial.println(nameLen);
    
    Serial.print("NAME: ");
    Serial.println(name);
    
    Serial.print("SKU: ");
    Serial.println((long)sku);
    
    Serial.print("VERSION: ");
    Serial.println(version, 2);
}

// END FILE