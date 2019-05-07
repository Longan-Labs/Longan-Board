#include <Wire.h>
#include <EEPROM.h>
#include "cardBasic.h"
#include "LonganCards.h"
#include "CardsDfs.h"

unsigned char dtaCnt = 0;       // count of sensor

int blinkCnt = 0;
CARD_INFO cardInfo;

unsigned char i2cDtaRaw[20];
unsigned char i2cDtaLen = 0;

float valueSensor[10];          // 10 value most
unsigned char dtaType[10];
unsigned char dtaUnit[10];

void cardInit()
{
    pinMode(2, OUTPUT);
    
    Wire.begin(cardInfo.addr);              // join i2c bus with address #8
    Wire.onReceive(receiveEvent);           // register event
    Wire.onRequest(requestEvent);           // register event
}

void blinkStart()
{
    blinkCnt = 10;
}

void blinkProcess()
{
    if(blinkCnt == 0)return;
    
    static unsigned long timer_s = millis();
    if(millis()-timer_s < 5)return;
    timer_s = millis();
    
    if(blinkCnt == 10)digitalWrite(2, HIGH);
    
    blinkCnt--;
    
    if(blinkCnt == 0)digitalWrite(2, LOW);
}


// get data from master
void receiveEvent(int howMany)
{
    blinkStart();
    while(Wire.available()>0)
    {
        i2cDtaRaw[i2cDtaLen++] = Wire.read();
    }
}


void requestEvent()
{
    requestFun();
}


void requestFun()
{
    blinkStart();
    
    if(i2cDtaLen == 1)
    {

        i2cDtaLen = 0;

        if(i2cDtaRaw[0] < REG_VALUE_START)     // get info
        {
            switch(i2cDtaRaw[0])
            {
                case REG_DEVIC:
                
                Serial.println("GET NUMBER");
                Wire.write(cardInfo.deviceID);
                break;
                
                case REG_SENSOR_CNT:
                Serial.println("GET CNT");
                Wire.write(cardInfo.senCnt);
                break;
                
                case REG_NAME_LEN:
                Serial.println("GET NAME LEN");
                Wire.write(cardInfo.nameLen);
                break;
                
                case REG_ADDR:
                Serial.println("GET ADDR");
                Wire.write(cardInfo.addr);
                break;
                
                case REG_NAME:
 
                Wire.write(cardInfo.name, cardInfo.nameLen);

                break;
                
                case REG_SKU:
                
                Serial.println("GET SKU");
                    {
                        char str[4];
                        float2str(cardInfo.sku, str);

                        Wire.write(str, 4);
                    }
                
                break;
                
                case REG_VERISON:
                
                Serial.println("GET VERSION");
                    {
                        char str[4];
                        float2str(cardInfo.version, str);
                        Wire.write(str, 4);
                    }
                
                break;
                
                default:
                break;
            }
        }
        else        // get sensor value
        {
            unsigned char valueN = i2cDtaRaw[0] - REG_VALUE_START;
            
            unsigned char rt[6];
            memset(rt, 0, 6);

            rt[0] = dtaType[valueN];
            float2str((float)valueSensor[valueN], &rt[1]);
            rt[5] = dtaUnit[valueN];
            
            Wire.write(rt, 6);

        }
    }
}

void float2str(float num, unsigned char *str)
{
    memcpy(str, (unsigned char*)(&num), 4);
}

float str2float(unsigned char *str)
{
    float num = 0;
    memcpy((unsigned char *)(&num), str, 4);
    return num;
}