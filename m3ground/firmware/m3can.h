#ifndef M3CAN_H
#define M3CAN_H
#include <stddef.h>

#define CAN_MSG_ID_M3RADIO_SI4460_CFG 0

void m3can_send_u8(uint16_t msg_id, uint8_t d0, uint8_t d1, uint8_t d2,
                   uint8_t d3, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7,
                   size_t n);

#endif
