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
#define CAN_MSG_ID_M3RADIO_GPS_LATLNG       (CAN_ID_M3RADIO | CAN_MSG_ID(48))
#define CAN_MSG_ID_M3RADIO_GPS_ALT          (CAN_ID_M3RADIO | CAN_MSG_ID(49))
#define CAN_MSG_ID_M3RADIO_GPS_TIME         (CAN_ID_M3RADIO | CAN_MSG_ID(50))
#define CAN_MSG_ID_M3RADIO_GPS_STATUS       (CAN_ID_M3RADIO | CAN_MSG_ID(51))


/* M3PSU */
#define CAN_MSG_ID_M3PSU_BATT_VOLTAGES      (CAN_ID_M3PSU | CAN_MSG_ID(1))


/* M3FC */
#define CAN_MSG_ID_M3FC_MISSION_STATE       (CAN_ID_M3FC | CAN_MSG_ID(32))
#define CAN_MSG_ID_M3FC_ACCEL               (CAN_ID_M3FC | CAN_MSG_ID(48))
#define CAN_MSG_ID_M3FC_BARO                (CAN_ID_M3FC | CAN_MSG_ID(49))
#define CAN_MSG_ID_M3FC_SE_T_H              (CAN_ID_M3FC | CAN_MSG_ID(50))
#define CAN_MSG_ID_M3FC_SE_V_A              (CAN_ID_M3FC | CAN_MSG_ID(51))
#define CAN_MSG_ID_M3FC_SE_VAR_H            (CAN_ID_M3FC | CAN_MSG_ID(52))
#define CAN_MSG_ID_M3FC_SE_VAR_V_A          (CAN_ID_M3FC | CAN_MSG_ID(53))
#define CAN_MSG_ID_M3FC_CFG_PROFILE         (CAN_ID_M3FC | CAN_MSG_ID(54))
#define CAN_MSG_ID_M3FC_CFG_PYROS           (CAN_ID_M3FC | CAN_MSG_ID(55))
#define CAN_MSG_ID_M3FC_SET_CFG_PROFILE     (CAN_ID_M3FC | CAN_MSG_ID(1))
#define CAN_MSG_ID_M3FC_SET_CFG_PYROS       (CAN_ID_M3FC | CAN_MSG_ID(2))
#define CAN_MSG_ID_M3FC_LOAD_CFG            (CAN_ID_M3FC | CAN_MSG_ID(3))
#define CAN_MSG_ID_M3FC_SAVE_CFG            (CAN_ID_M3FC | CAN_MSG_ID(4))


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

/* Type specific can_send functions */
void can_send_u8(uint16_t msg_id, uint8_t d0, uint8_t d1, uint8_t d2,
                 uint8_t d3, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7,
                 size_t n);
void can_send_u16(uint16_t msg_id, uint16_t d0, uint16_t d1, uint16_t d2,
                  uint16_t d3, size_t n);
void can_send_u32(uint16_t msg_id, uint32_t d0, uint32_t d1, size_t n);
void can_send_i8(int16_t msg_id, int8_t d0, int8_t d1, int8_t d2,
                 int8_t d3, int8_t d4, int8_t d5, int8_t d6, int8_t d7,
                 size_t n);
void can_send_i16(int16_t msg_id, int16_t d0, int16_t d1, int16_t d2,
                  int16_t d3, size_t n);
void can_send_i32(int16_t msg_id, int32_t d0, int32_t d1, size_t n);
void can_send_f32(uint16_t msg_id, float d0, float d1, size_t n);

/* Whether to process all sent messages as though they were also received */
void can_set_loopback(bool enabled);

#endif /* _M3CAN_H */
