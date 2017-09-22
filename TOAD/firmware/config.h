#ifndef CONFIG_H
#define CONFIG_H

#include "microsd.h"

/*          GLOABAL CONFIG SETTINGS

    DEVICE  TYPE    TOAD_ID     BACKHAUL_DELAY    

    TOAD_1   S      0x01        150 ms
    TOAD_2   S      0x02        350 ms
    TOAD_3   S      0x04        550 ms
    TOAD_4   S      0x08        750 ms
    TOAD_5   S      0x10        950 ms
    TOAD_6   M      0x20        N/A

*/

/* Global TOAD ID Constants */
#define TOAD_1_ID       0x01
#define TOAD_2_ID       0x02
#define TOAD_3_ID       0x04
#define TOAD_4_ID       0x08
#define TOAD_5_ID       0x10
#define TOAD_MASTER_ID  0x20

/* Global TOAD Backhaul Delays */
#define TOAD_1_DELAY        150
#define TOAD_2_DELAY        350
#define TOAD_3_DELAY        550
#define TOAD_4_DELAY        750
#define TOAD_5_DELAY        950

typedef struct __attribute__((packed)) {
    uint8_t id;
    uint16_t delay;
    bool configured;
} toad_config;

/* Toad Config */
extern toad_config toad;

/* Configure Toad */
void configure_toad(SDFS* file_system);

#endif 
