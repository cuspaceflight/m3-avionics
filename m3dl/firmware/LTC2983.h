#ifndef LTC2983_H
#define LTC2983_H

/* LTC2983 Registers */
#define CMD_STATUS_REG      0x000
#define TEMP_RES_REG        0x010
#define CHANNEL_MASK_REG    0x0F5
#define CHANNEL_ASSIGN_REG  0x200

/* Channel Masks */
#define TEMP1_MASK   0x02
#define TEMP2_MASK   0x08
#define TEMP3_MASK   0x20
#define TEMP4_MASK   0x80
#define TEMP5_MASK   0x02
#define TEMP6_MASK   0x08
#define TEMP7_MASK   0x20
#define TEMP8_MASK   0x80
#define TEMP9_MASK   0x02

/* USER DEFINED CONFIG */
#define TEMP1_ATTACHED      0
#define TEMP2_ATTACHED      1
#define TEMP3_ATTACHED      0
#define TEMP4_ATTACHED      0
#define TEMP5_ATTACHED      0
#define TEMP6_ATTACHED      0
#define TEMP7_ATTACHED      0
#define TEMP8_ATTACHED      0
#define TEMP9_ATTACHED      0

#define NUM_ATTACHED        1

/* Init LTC2983 */
void ltc2983_init(void);

/* Interrupt Callback */
void temp_ready(EXTDriver *extp, expchannel_t channel);

#endif
