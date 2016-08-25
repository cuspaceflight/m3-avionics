#ifndef _M3CAN_H
#define _M3CAN_H

#include "ch.h"
#include "hal.h"

#define CAN_ID_M3FC      (1)
#define CAN_ID_M3PSU     (2)
#define CAN_ID_M3PYRO    (3)
#define CAN_ID_M3RADIO   (4)
#define CAN_ID_M3IMU     (5)
#define CAN_ID_M3DL      (6)

#define CAN_MSG_ID(x)    (x<<5)

#define CAN_MSG_ID_VERSION                  CAN_MSG_ID(63)
#define CAN_MSG_ID_STATUS                   CAN_MSG_ID(0)

extern uint8_t m3can_own_id;

// Fill out known CAN message IDs here, so all boards can know about them
/* M3RADIO */


/* M3PSU */
#define CAN_MSG_ID_M3PSU_BATT_VOLTAGES      (CAN_ID_M3PSU | CAN_MSG_ID(1))


/* M3FC */
#define CAN_MSG_ID_M3FC_MISSION_STATE       (CAN_ID_M3FC | CAN_MSG_ID(32))
#define CAN_MSG_ID_M3FC_ACCEL               (CAN_ID_M3FC | CAN_MSG_ID(48))
#define CAN_MSG_ID_M3FC_BARO                (CAN_ID_M3FC | CAN_MSG_ID(49))
#define CAN_MSG_ID_M3FC_SE_H_V              (CAN_ID_M3FC | CAN_MSG_ID(50))
#define CAN_MSG_ID_M3FC_SE_A                (CAN_ID_M3FC | CAN_MSG_ID(51))
#define CAN_MSG_ID_M3FC_CFG_PROFILE         (CAN_ID_M3FC | CAN_MSG_ID(52))
#define CAN_MSG_ID_M3FC_CFG_PYROS           (CAN_ID_M3FC | CAN_MSG_ID(53))


/* M3DL */


/* M3IMU */


/* M3PYRO */
#define CAN_MSG_ID_M3PYRO_FIRE_COMMAND      (CAN_ID_M3PYRO | CAN_MSG_ID(1))
#define CAN_MSG_ID_M3PYRO_ARM_COMMAND       (CAN_ID_M3PYRO | CAN_MSG_ID(2))
#define CAN_MSG_ID_M3PYRO_FIRE_STATUS       (CAN_ID_M3PYRO | CAN_MSG_ID(16))
#define CAN_MSG_ID_M3PYRO_ARM_STATUS        (CAN_ID_M3PYRO | CAN_MSG_ID(17))
#define CAN_MSG_ID_M3PYRO_CONTINUITY        (CAN_ID_M3PYRO | CAN_MSG_ID(48))
#define CAN_MSG_ID_M3PYRO_SUPPLY_STATUS     (CAN_ID_M3PYRO | CAN_MSG_ID(49))



// Define this function somewhere else and fill it out
void can_recv(uint16_t msg_id, bool can_rtr, uint8_t *data, uint8_t datalen);

void can_init(uint8_t board_id);
void can_send(uint16_t msg_id, bool can_rtr, uint8_t *data, uint8_t datalen);

#endif /* _M3CAN_H */
