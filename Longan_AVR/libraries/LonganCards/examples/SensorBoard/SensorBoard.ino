// get data from card sensor

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "LonganCards.h"
#include "CardsDfs.h"
#include "cardBasic.h"
#include "watchdog.h"

CARD_INFO card;

const int pinBtnL   = A0;
const int pinBtnR   = A1;

// Carduino 32U4
#define OLED_DC     4          // INT
#define OLED_CS     12          // OK
#define OLED_RESET  6           // FAIL

Adafruit_SSD1306 display(OLED_DC, OLED_RESET, OLED_CS);

unsigned char sensorVal[10][6];
unsigned char addr_buf = 0;

unsigned char cntDisp = 0;

// MODE
// 0 - all in one page
// 1 - roll
// 2 - right button to switch
unsigned char dispMode  = 1;
unsigned char switchCnt = 0;

void oledInit()
{
    display.begin(SSD1306_SWITCHCAPVCC);
    display.display();
    delay(2000);
}

void getSensor()
{
    card.addr = getAddr();
    
    if(card.addr == 0)
    {
        Serial.println("NO DEVICE");
        waitForDevice();
        return;
    }
    
    unsigned char dta[10];
    unsigned char dtaLen = 0;

    for(int i=0; i<card.senCnt; i++)
    {
        Wire.beginTransmission(card.addr); // transmit to device #8
        Wire.write(0x30+i);  
        Wire.endTransmission();    // stop transmitting
        delay(10);
        WDR();
        Wire.requestFrom(card.addr, 6);    // request 6 bytes from slave device #8
        delay(10);
        WDR();
        while (Wire.available())
        { // slave may send less than requested
            dta[dtaLen++] = Wire.read();
        }
        
        if(dtaLen)
        {
            for(int j=0; j<dtaLen; j++)
            {
                sensorVal[i][j] = dta[j];
            }
            memset(dta, 0, 10);
            dtaLen = 0;
        }
        delay(10);
    }
}

void modeSwitchBtn()
{
    if(!digitalRead(pinBtnL))
    {
        delay(10);
        if(!digitalRead(pinBtnL))
        {
            cntDisp = 0;
            dispMode = (dispMode == 2) ? 0 : (1-dispMode);
            while(!digitalRead(pinBtnL));
            
            Serial.print("MODE = ");
            Serial.println(dispMode);
        }
    }
    
    if(!digitalRead(pinBtnR))
    {
        delay(10);
        if(!digitalRead(pinBtnR))
        {
            if(dispMode == 2)
            {
                switchCnt++;
                if(switchCnt>=card.senCnt)
                {
                    switchCnt = 0;
                }
            }
            else
            {
                dispMode = 2;
                switchCnt = 0;
            }
            
            while(!digitalRead(pinBtnR));
            Serial.print("SWITCH = ");
            Serial.println(switchCnt);
        }
    }
}



void dispModeAll()
{
    
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    
    for(int i=0; i<card.senCnt; i++)
    {
        display.setCursor(0, 10*i);
        
        switch(sensorVal[i][5])
        {
            case UNIT_TEMPERATURE:
            dispSensorSmall(str2float(&sensorVal[i][1]), UNIT_TEMPERATURE, 1, "Temperature");
            break;
            
            case UNIT_HUMIDITY:
            dispSensorSmall(str2float(&sensorVal[i][1]), UNIT_HUMIDITY, 0, "Humidity");
            break;
            
            case UNIT_METER:
            dispSensorSmall(str2float(&sensorVal[i][1]), UNIT_METER, 1, "Meter");
            break;
            
            case UNIT_MILLIMETER:
            dispSensorSmall(str2float(&sensorVal[i][1]), UNIT_MILLIMETER, 0, "Millimeter");
            break;
            
            case UNIT_PRESS:
            dispSensorSmall(str2float(&sensorVal[i][1]), UNIT_PRESS, 3, "Press");
            break;
            
            case UNIT_ANALOG:
            dispSensorSmall(str2float(&sensorVal[i][1]), UNIT_ANALOG, 0, "Value");
            break;
            
            //case UNIT_ALTITUDE:
            //dispSensorSmall(str2float(&sensorVal[i][1]), UNIT_ALTITUDE, 1, "Altitude");
            //break;
         
            default:;
        }
    }
    
    display.display();
    
    display.display();
}

void dispModeRoll()
{
    if(card.addr == 0)return;
    static unsigned long timer_s = millis();
    
    if(millis()-timer_s > 3000)
    {
        cntDisp++;
        cntDisp = (cntDisp >= card.senCnt) ? 0 : cntDisp;
        timer_s = millis();
    }
    
    switch(sensorVal[cntDisp][5])
    {
        case UNIT_TEMPERATURE:
        dispSensor(str2float(&sensorVal[cntDisp][1]), UNIT_TEMPERATURE, 1, "Temperature");
        break;
        
        case UNIT_HUMIDITY:
        dispSensor(str2float(&sensorVal[cntDisp][1]), UNIT_HUMIDITY, 0, "Humidity");
        break;
        
        case UNIT_METER:
        dispSensor(str2float(&sensorVal[cntDisp][1]), UNIT_METER, 1, "Meter");
        break;
        
        case UNIT_MILLIMETER:
        dispSensor(str2float(&sensorVal[cntDisp][1]), UNIT_MILLIMETER, 0, "Millimeter");
        break;
        
        case UNIT_PRESS:
        dispSensor(str2float(&sensorVal[cntDisp][1]), UNIT_PRESS, 3, "Press");
        break;
        
        case UNIT_ANALOG:
        dispSensor(str2float(&sensorVal[cntDisp][1]), UNIT_ANALOG, 0, "Value");
        break;
        
        //case UNIT_ALTITUDE:
        //dispSensor(str2float(&sensorVal[cntDisp][1]), UNIT_ALTITUDE, 1, "Altitude");
        //break;
     
        default:;
    }
}

void dispModeSwitch()
{
    if(card.addr == 0)return;

    switch(sensorVal[switchCnt][5])
    {
        case UNIT_TEMPERATURE:
        dispSensor(str2float(&sensorVal[switchCnt][1]), UNIT_TEMPERATURE, 1, "Temperature");
        break;
        
        case UNIT_HUMIDITY:
        dispSensor(str2float(&sensorVal[switchCnt][1]), UNIT_HUMIDITY, 0, "Humidity");
        break;
        
        case UNIT_METER:
        dispSensor(str2float(&sensorVal[switchCnt][1]), UNIT_METER, 1, "Meter");
        break;
        
        case UNIT_MILLIMETER:
        dispSensor(str2float(&sensorVal[switchCnt][1]), UNIT_MILLIMETER, 0, "Millimeter");
        break;
        
        case UNIT_PRESS:
        dispSensor(str2float(&sensorVal[switchCnt][1]), UNIT_PRESS, 3, "Press");
        break;
        
        case UNIT_ANALOG:
        dispSensor(str2float(&sensorVal[switchCnt][1]), UNIT_ANALOG, 0, "");
        break;
        
        //case UNIT_ALTITUDE:
        //dispSensor(str2float(&sensorVal[switchCnt][1]), UNIT_ALTITUDE, 1, "Altitude");
        //break;
     
        default:;
    }
}

void displaySensor()
{
    
    switch (dispMode)
    {
        case 0:         // all in one page
        
        dispModeAll();
        break;
        case 1:         // rool
        dispModeRoll();
        break;
        case 2:         // switch to change
        dispModeSwitch();       
        
        break;
        
        default:;
        
    }

    
}

unsigned char getAddr()
{
    static long timer_s = -10001;
    if(millis()-timer_s < 1000)return card.addr;
    timer_s = millis();
    
    for(int i = 1; i < 127; i++ )
    {
        Wire.beginTransmission(i);
        unsigned char error = Wire.endTransmission();
 
        if (error == 0)
        {
            return i;
        }  
    }
    
    return 0;    
}

void waitForDevice()
{
    while(1)
    {
        Serial.println("GET ADDR FROM THE CARD");
        card.addr = getAddr();
        Serial.print("CARD ADDR = 0X");
        Serial.println(card.addr, HEX);
        
        if(card.addr == 0)
        {
            Serial.println("NO DEVICE!");
            
            display.clearDisplay();
            display.setCursor(0, 0);
            display.setTextSize(1);
            display.setTextColor(WHITE);
            display.println("NO DEVICE");
            display.display();
            
            delay(500);
            WDR();
        }
        else
        {
            if(card.addr != addr_buf)
            {
                addr_buf = card.addr;
                card.getInfo();
                card.disp();
            }
            break;
        }
        
        blinkblink();
    }
}

void setup() {
    
    Wire.begin();        // join i2c bus (address optional for master)
    Serial.begin(115200);  // start serial for output
    
   // while(!Serial.available());
    
    Serial.println("start");
    
    pinMode(pinBtnL, INPUT_PULLUP);
    pinMode(pinBtnR, INPUT_PULLUP);
    pinMode(13, OUTPUT);
    
    //card.addr = getAddr();
    oledInit();
    
    Serial.println("OLED INIT OK");
    
    waitForDevice();
    
    WD_SET(WD_RST, WDTO_1S);
    
    Serial.println("init done");

}

void loop() {

    blinkblink();
    WDR();
    modeSwitchBtn();
    WDR();
    getSensor();
    WDR();
    displaySensor();
    WDR();
    delay(50);
    //Serial.println("#");
    WDR();
}

void printTstr(int x, int y, int size, char *str)
{
    display.setTextSize(size);
    display.setCursor(x,y);
    display.println(str);
}

void printTnum(float num, int d)
{
    int tmp = num;
    
    int a = 0;
    
    
    do
    {
        tmp /= 10;
        a++;
    }while(tmp);
    
    a += (d>0) ? (d+1) : 0;
    
    int x = (128-a*18.2)/2;
    int y = 20;
    
    display.setTextSize(3);
    display.setCursor(x, y);
    display.println(num, d);
}


void printUnit(int unit)
{
    switch (unit)
    {
        case UNIT_TEMPERATURE:

        printTstr(80, 48, 1, "o");
        printTstr(88, 50, 2, "C");
        break;
        
        case UNIT_HUMIDITY:
        
        printTstr(88, 50, 2, "%");
        break;
        
        case UNIT_METER:
        //case UNIT_ALTITUDE:
        
        printTstr(88, 50, 2, "m");
        break;
        
        case UNIT_MILLIMETER:
        
        printTstr(88, 50, 2, "mm");
        break;
        
        case UNIT_PRESS:
        
        printTstr(88, 50, 2, "kPa");
        break;
        
        
        default:;
    }
}

void printUnitSmall(int unit)
{
    switch (unit)
    {
        case UNIT_TEMPERATURE:

        display.print("C");
        break;
        
        case UNIT_HUMIDITY:
        
        display.print("%");
        break;
        
        case UNIT_METER:
        //case UNIT_ALTITUDE:
        
        display.print("m");
        break;
        
        case UNIT_MILLIMETER:
        
        display.print("mm");
        break;
        
        case UNIT_PRESS:
        
        display.print("kPa");
        break;
        
        
        default:;
    }
}


void dispSensor(float num, unsigned char unit, unsigned char d, char *str)
{
    display.clearDisplay();
    display.setTextColor(WHITE);
    
    
    printTstr(0, 0, 1, str);
    printTnum(num, d);
    printUnit(unit);
    display.display();
}


void dispSensorSmall(float num, unsigned char unit, unsigned char d, char *str)
{
    //display.setCursor(0, 0);
    display.print(str);
    display.print(" = ");
    display.print(num, d);
    display.print(" ");
    printUnitSmall(unit);
    display.println();
}

void blinkblink()
{
    static unsigned long timer_s = millis();
    
    if(millis()-timer_s < 100)return;
    timer_s = millis();
    
    digitalWrite(13, 1-digitalRead(13));
    
}
// END FILE
