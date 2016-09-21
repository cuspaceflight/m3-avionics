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
#define CAN_MSG_ID_M3PSU_BATT_VOLTAGES          (CAN_ID_M3PSU | CAN_MSG_ID(56))
#define CAN_MSG_ID_M3PSU_TOGGLE_PYROS           (CAN_ID_M3PSU | CAN_MSG_ID(16))
#define CAN_MSG_ID_M3PSU_CHANNEL_STATUS_12      (CAN_ID_M3PSU | CAN_MSG_ID(49))
#define CAN_MSG_ID_M3PSU_CHANNEL_STATUS_34      (CAN_ID_M3PSU | CAN_MSG_ID(50))
#define CAN_MSG_ID_M3PSU_CHANNEL_STATUS_56      (CAN_ID_M3PSU | CAN_MSG_ID(51))
#define CAN_MSG_ID_M3PSU_CHANNEL_STATUS_78      (CAN_ID_M3PSU | CAN_MSG_ID(52))
#define CAN_MSG_ID_M3PSU_CHANNEL_STATUS_910     (CAN_ID_M3PSU | CAN_MSG_ID(53))
#define CAN_MSG_ID_M3PSU_CHANNEL_STATUS_1112    (CAN_ID_M3PSU | CAN_MSG_ID(54))
#define CAN_MSG_ID_M3PSU_TOGGLE_CHANNEL         (CAN_ID_M3PSU | CAN_MSG_ID(17))
#define CAN_MSG_ID_M3PSU_PYRO_STATUS            (CAN_ID_M3PSU | CAN_MSG_ID(48))
#define CAN_MSG_ID_M3PSU_CHARGER_STATUS         (CAN_ID_M3PSU | CAN_MSG_ID(55))
#define CAN_MSG_ID_M3PSU_TOGGLE_CHARGER         (CAN_ID_M3PSU | CAN_MSG_ID(18))
#define CAN_MSG_ID_M3PSU_TOGGLE_BALANCE         (CAN_ID_M3PSU | CAN_MSG_ID(19))
#define CAN_MSG_ID_M3PSU_TOGGLE_INTEXT          (CAN_ID_M3PSU | CAN_MSG_ID(20))
#define CAN_MSG_ID_M3PSU_INTEXT_STATUS          (CAN_ID_M3PSU | CAN_MSG_ID(57))


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
#define CAN_MSG_ID_M3FC_MOCK_ENABLE         (CAN_ID_M3FC | CAN_MSG_ID(5))
#define CAN_MSG_ID_M3FC_MOCK_ACCEL          (CAN_ID_M3FC | CAN_MSG_ID(6))
#define CAN_MSG_ID_M3FC_MOCK_BARO           (CAN_ID_M3FC | CAN_MSG_ID(7))
#define CAN_MSG_ID_M3FC_ARM                 (CAN_ID_M3FC | CAN_MSG_ID(8))


/* M3DL */


/* M3IMU */


/* M3PYRO */
#define CAN_MSG_ID_M3PYRO_FIRE_COMMAND      (CAN_ID_M3PYRO | CAN_MSG_ID(1))
#define CAN_MSG_ID_M3PYRO_ARM_COMMAND       (CAN_ID_M3PYRO | CAN_MSG_ID(2))
#define CAN_MSG_ID_M3PYRO_FIRE_STATUS       (CAN_ID_M3PYRO | CAN_MSG_ID(16))
#define CAN_MSG_ID_M3PYRO_ARM_STATUS        (CAN_ID_M3PYRO | CAN_MSG_ID(17))
#define CAN_MSG_ID_M3PYRO_CONTINUITY        (CAN_ID_M3PYRO | CAN_MSG_ID(48))
#define CAN_MSG_ID_M3PYRO_SUPPLY_STATUS     (CAN_ID_M3PYRO | CAN_MSG_ID(49))



/* Define this function somewhere else and implement it */
void can_recv(uint16_t msg_id, bool can_rtr, uint8_t *data, uint8_t datalen);

/* Call can_init early during startup, setting your board ID from the list
 * above.
 */
void can_init(uint8_t board_id);

/* Call can_send to transmit a packet.
 * msg_id should be from the list above
 * can_rtr is the "remote transmission request", set to indicate you're asking
 *         for data rather than sending it
 * data[] and datalen are the data (and number of bytes of it) to send.
 */
void can_send(uint16_t msg_id, bool can_rtr, uint8_t *data, uint8_t datalen);

/* Type specific can_send functions. In each case 'n' refers to the number of
 * values transmitted, not the number of corresponding bytes, so e.g. should be
 * 0 or 1 for send_u32. 
 */
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

/* Enable processing all sent messages (via can_send) as though they were also
 * received (in can_recv).
 * Useful for boards like datalogger and radio which want to receive their own
 * packets for storage/transmission.
 */
void can_set_loopback(bool enabled);

#endif /* _M3CAN_H */

