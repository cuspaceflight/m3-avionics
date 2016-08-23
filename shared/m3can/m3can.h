#ifndef _M3CAN_H
#define _M3CAN_H

#include "ch.h"
#include "hal.h"

#define CAN_ID_M3RADIO   (1<<6)
#define CAN_ID_M3PSU     (2<<6)
#define CAN_ID_M3FC      (3<<6)
#define CAN_ID_M3DL      (4<<6)
#define CAN_ID_M3IMU     (5<<6)
#define CAN_ID_M3PYRO    (6<<6)

// Fill out known CAN message IDs here, so all boards can know about them
/* M3RADIO */


/* M3PSU */
#define CAN_MSG_ID_M3PSU_BATT_VOLTAGES      (CAN_ID_M3PSU | 1)
#define CAN_MSG_ID_M3PSU_TOGGLE_PYROS       (CAN_ID_M3PSU | 2)
#define CAN_MSG_ID_M3PSU_CHANNEL_STATUS_12  (CAN_ID_M3PSU | 3)
#define CAN_MSG_ID_M3PSU_CHANNEL_STATUS_34  (CAN_ID_M3PSU | 4)
#define CAN_MSG_ID_M3PSU_CHANNEL_STATUS_56  (CAN_ID_M3PSU | 5)
#define CAN_MSG_ID_M3PSU_CHANNEL_STATUS_78  (CAN_ID_M3PSU | 6)
#define CAN_MSG_ID_M3PSU_CHANNEL_STATUS_910  (CAN_ID_M3PSU | 7)
#define CAN_MSG_ID_M3PSU_CHANNEL_STATUS_1112  (CAN_ID_M3PSU | 8)
#define CAN_MSG_ID_M3PSU_TOGGLE_CHANNEL     (CAN_ID_M3PSU | 9)
#define CAN_MSG_ID_M3PSU_PYRO_STATUS        (CAN_ID_M3PSU | 10)
#define CAN_MSG_ID_M3PSU_CHARGER_STATUS     (CAN_ID_M3PSU | 11)
#define CAN_MSG_ID_M3PSU_TOGGLE_CHARGER     (CAN_ID_M3PSU | 12)
#define CAN_MSG_ID_M3PSU_TOGGLE_BALANCE     (CAN_ID_M3PSU | 13)


/* M3FC */


/* M3DL */


/* M3IMU */


/* M3PYRO */



// Define this function somewhere else and fill it out
void can_recv(uint16_t msg_id, bool can_rtr, uint8_t *data, uint8_t datalen);

void can_init(void);
void can_send(uint16_t msg_id, bool can_rtr, uint8_t *data, uint8_t datalen);

#endif /* _M3CAN_H */

