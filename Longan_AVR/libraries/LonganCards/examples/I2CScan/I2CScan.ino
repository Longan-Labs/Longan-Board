#include <Wire.h>

void setup()
{
    Wire.begin();

    Serial.begin(115200);
    while (!Serial); // Leonardo: wait for serial monitor
    Serial.println("begn to scan...");
}


void loop()
{
    int nCards = 0;

    for (unsigned char i = 1; i <= 127; i++)
    {
        Wire.beginTransmission(i);
        byte ret = Wire.endTransmission();

        if (ret == 0) // success
        {
        Serial.print("0x");
        Serial.println(i, HEX);
        nCards++; // count number of cards
        }   
    }

    if (nCards == 0)
        Serial.println("find nothing");
    
    while (1);
}