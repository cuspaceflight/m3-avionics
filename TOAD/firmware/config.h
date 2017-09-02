#ifndef CONFIG_H
#define CONFIG_H

/*          GLOABAL CONFIG SETTINGS

    DEVICE  TYPE    TOAD_ID     BACKHAUL_DELAY    

    TOAD_1   S      0x01        150
    TOAD_2   S      0x02        350
    TOAD_3   S      0x04        550
    TOAD_4   S      0x08        750
    TOAD_5   S      0x10        950
    TOAD_6   M      0x20        N/A

*/


/* BOARD ID */
#define TOAD_ID         0x20

/* BOARD Backhaul Delay */
#define BACKHAUL_DELAY  350


/* Global TOAD Constants */
#define TOAD_1          0x01
#define TOAD_2          0x02
#define TOAD_3          0x04
#define TOAD_4          0x08
#define TOAD_5          0x10
#define TOAD_MASTER     0x20


#endif 
