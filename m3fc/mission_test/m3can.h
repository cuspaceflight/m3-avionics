#pragma once

#define m3can_init(x)
#define m3can_send(a, b, c, d) ((void)c)

#define CAN_ID_M3FC      (1)
#define CAN_ID_M3RADIO   (4)
#define CAN_MSG_ID(x)    (x<<5)
#define CAN_MSG_ID_M3FC_MISSION_STATE       (CAN_ID_M3FC | CAN_MSG_ID(32))
#define CAN_MSG_ID_M3FC_ACCEL               (CAN_ID_M3FC | CAN_MSG_ID(48))
#define CAN_MSG_ID_M3FC_BARO                (CAN_ID_M3FC | CAN_MSG_ID(49))
#define CAN_MSG_ID_M3FC_SE_T_H              (CAN_ID_M3FC | CAN_MSG_ID(50))
#define CAN_MSG_ID_M3FC_SE_V_A              (CAN_ID_M3FC | CAN_MSG_ID(51))
#define CAN_MSG_ID_M3FC_SE_VAR_H            (CAN_ID_M3FC | CAN_MSG_ID(52))
#define CAN_MSG_ID_M3FC_SE_VAR_V_A          (CAN_ID_M3FC | CAN_MSG_ID(53))
#define CAN_MSG_ID_M3RADIO_GPS_ALT          (CAN_ID_M3RADIO | CAN_MSG_ID(49))
