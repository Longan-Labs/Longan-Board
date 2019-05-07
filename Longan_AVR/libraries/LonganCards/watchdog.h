#ifndef __WATCHDOG_H__
#define __WATCHDOG_H__

#include <avr/io.h>

// watch dog
#define WD_SET(val,...)                                 \
    __asm__ __volatile__(                               \
        "in __tmp_reg__,__SREG__"           "\n\t"      \
        "cli"                               "\n\t"      \
        "wdr"                               "\n\t"      \
        "sts %[wdreg],%[wden]"              "\n\t"      \
        "sts %[wdreg],%[wdval]"             "\n\t"      \
        "out __SREG__,__tmp_reg__"          "\n\t"      \
        :                                               \
        : [wdreg] "M" (&WDTCSR),                        \
          [wden]  "r" ((uint8_t)(0x18)),                \
          [wdval] "r" ((uint8_t)(val|(__VA_ARGS__+0)))  \
        : "r0"                                          \
)
    
    
#define WDTO_15MS               0
#define WDTO_30MS               1
#define WDTO_60MS               2
#define WDTO_120MS              3
#define WDTO_250MS              4
#define WDTO_500MS              5
#define WDTO_1S                 6
#define WDTO_2S                 7
#define WDTO_4S                 0x20
#define WDTO_8S                 0x21

#define WD_RST                  8
#define WDR()                   __asm__ __volatile__("wdr")

#endif