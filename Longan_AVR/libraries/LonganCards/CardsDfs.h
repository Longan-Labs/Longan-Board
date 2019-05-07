// cards defines
#ifndef __CARDSDFS_H__
#define __CARDSDFS_H__


#define REG_DEVIC           0X01
#define REG_SENSOR_CNT      0X02
#define REG_NAME_LEN        0X03
#define REG_ADDR            0X04

#define REG_NAME            0X10
#define REG_SKU             0X11
#define REG_VERISON         0X12

#define REG_VALUE_START     0X30


// DATA TYPE
#define DTA_TYPE_INT    1
#define DTA_TYPE_FLOAT  2


// DATA UNIT
#define UNIT_TEMPERATURE        1       // ℃
#define UNIT_HUMIDITY           2       // %
#define UNIT_METER              3       // m
#define UNIT_MILLIMETER         4       // mm
#define UNIT_PRESS              5       // PA
#define UNIT_ANALOG             6       // A NUMBER BETWEEN 0~1023
#define UNIT_GESTURE            7       // GESTURE
#define UNIT_ALTITUDE           8       // m

// GESTURE
#define GESTURE_UP              1
#define GESTURE_DOWN            2
#define GESTURE_LEFT            3
#define GESTURE_RIGHT           4
#define GESTURE_FORWARD         5
#define GESTURE_BACKWARD        6
#define GESTURE_CLOCKWISE       7
#define GESTURE_ANTICLOCK       8
#define GESTURE_WAVE            9



#endif