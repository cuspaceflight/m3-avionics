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


/* M3FC */


/* M3DL */


/* M3IMU */


/* M3PYRO */
#define CAN_MSG_ID_M3PYRO_ARM_STATUS        (CAN_ID_M3PYRO | 1)
#define CAN_MSG_ID_M3PYRO_ARM_COMMAND       (CAN_ID_M3PYRO | 2)
#define CAN_MSG_ID_M3PYRO_CONTINUITY        (CAN_ID_M3PYRO | 3)
#define CAN_MSG_ID_M3PYRO_FIRE_COMMAND      (CAN_ID_M3PYRO | 4)
#define CAN_MSG_ID_M3PYRO_SUPPLY_STATUS     (CAN_ID_M3PYRO | 5)



// Define this function somewhere else and fill it out
void can_recv(uint16_t msg_id, bool can_rtr, uint8_t *data, uint8_t datalen);

void can_init(void);
void can_send(uint16_t msg_id, bool can_rtr, uint8_t *data, uint8_t datalen);

#endif /* _M3CAN_H */
