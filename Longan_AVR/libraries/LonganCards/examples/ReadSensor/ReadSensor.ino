#include <Wire.h>
#include "CardsDfs.h"
#include "LonganCards.h"

CARD_INFO card;

int addr = 0;
int cntSensor = 0;

float sensorValue[10];

float str2num(unsigned char *str)
{
    float num = 0;
    memcpy((unsigned char *)(&num), str, 4);
    return num;
}

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    Wire.begin();
    
    while(!Serial.available());
    
    addr = card.scan();
    if(0 == addr)
    {
        Serial.println("NO DEVICE!");
        while(1);
    }
    
    card.getInfo();
    card.disp();
    
    cntSensor = card.senCnt;
}

void loop()
{
    for(int i=0; i<cntSensor; i++)
    {
        sensorValue[i] = getValue(i);
        Serial.print(sensorValue[i], 2);
        Serial.print('\t');
    }
    
    Serial.println();
    delay(100);
}

float getValue(int s)
{
    unsigned char dta[10];
    unsigned long len = 0;
    
    Wire.beginTransmission(addr);
    Wire.write(REG_VALUE_START+s);      // ADDR_DATA_START = 0x30
    Wire.endTransmission();
    
    delay(1);
    Wire.requestFrom(addr, 6);
    
    delay(1);
    
    while(Wire.available())
    {
        dta[len++] = Wire.read();
    }
    
    return str2num(&dta[1]);
}

// END FILE