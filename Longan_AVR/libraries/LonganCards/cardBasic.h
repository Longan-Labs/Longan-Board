// basic function
#ifndef __CARDBASIC_H__
#define __CARDBASIC_H__

#include <Arduino.h>
#include "CardsDfs.h"
#include "LonganCards.h"


extern float valueSensor[10];          // 10 value most
extern CARD_INFO cardInfo;

extern unsigned char dtaType[10];
extern unsigned char dtaUnit[10];
extern unsigned char dtaCnt;

extern void blinkStart();
extern void blinkProcess();
extern void cardInit();
extern void receiveEvent(int howMany);
extern void requestEvent();
extern void requestFun();
extern void sensorUpdate();

extern void float2str(float num, unsigned char *str);
extern float str2float(unsigned char *str);

#endif