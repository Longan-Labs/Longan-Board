#include <LonganCards.h>
#include <Wire.h>

CARD_INFO card;

void setup()
{
    Serial.begin(115200);
    Wire.begin();
    
    while(!Serial.available());     // input anything to start
    
    if(!card.scan())
    {
        Serial.println("NO DEVICE FOUND!");
        while(1);
    }
    
    card.getInfo();

    Serial.print("ADDR: 0x");
    Serial.println(card.addr, HEX);
    
    Serial.print("DEVICE ID.");
    Serial.println(card.deviceID);
    
    Serial.print("SENSOR CNT: ");
    Serial.println(card.senCnt);
    
    Serial.print("NAME LEN: ");
    Serial.println(card.nameLen);
    
    Serial.print("NAME: ");
    Serial.println(card.name);
    
    Serial.print("SKU: ");
    Serial.println((long)card.sku);
    
    Serial.print("VERSION: ");
    Serial.println(card.version, 2);
    
    
    Serial.print("Product Page: ");
    Serial.print("https://www.longan-labs.cc/");
    Serial.print((long)card.sku);
    Serial.println(".html");
    Serial.print("Wiki: ");
    Serial.print("https://docs.longan-labs.cc/");
    Serial.print((long)card.sku);
    Serial.println("/");
}

void loop()
{

}

// END FILE