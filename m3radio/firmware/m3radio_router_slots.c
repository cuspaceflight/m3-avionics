#include "m3radio_router_slots.h"

struct m3radio_slot m3radio_slots[2048] = {
    /* Unless otherwise specified, default is to never transmit. */
    {.mode=M3RADIO_ROUTER_MODE_NEVER, .skip_count=0},

    /* M3Radio Packets */
    [CAN_ID_M3RADIO | CAN_MSG_ID_VERSION]   = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 30000 },
    [CAN_ID_M3RADIO | CAN_MSG_ID_STATUS]    = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 1000 },
    [CAN_MSG_ID_M3RADIO_GPS_LATLNG] = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 1000 },
    [CAN_MSG_ID_M3RADIO_GPS_ALT]    = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 1000 },
    [CAN_MSG_ID_M3RADIO_GPS_TIME]   = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 1000 },
    [CAN_MSG_ID_M3RADIO_GPS_STATUS] = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 3000 },
    [CAN_MSG_ID_M3RADIO_SI4460_CFG] = { .mode = M3RADIO_ROUTER_MODE_NEVER },
    [CAN_MSG_ID_M3RADIO_PACKET_COUNT] = { .mode = M3RADIO_ROUTER_MODE_ALWAYS },
    [CAN_MSG_ID_M3RADIO_PACKET_STATS] = { .mode = M3RADIO_ROUTER_MODE_ALWAYS },


    /* M3PSU Packets */
    [CAN_ID_M3PSU | CAN_MSG_ID_VERSION] = { .mode = M3RADIO_ROUTER_MODE_ALWAYS },
    [CAN_ID_M3PSU | CAN_MSG_ID_STATUS]  = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 1000 },
    [CAN_MSG_ID_M3PSU_BATT_VOLTAGES]        = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 5000 },
    [CAN_MSG_ID_M3PSU_CHANNEL_STATUS_12]    = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 5000 },
    [CAN_MSG_ID_M3PSU_CHANNEL_STATUS_34]    = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 5000 },
    [CAN_MSG_ID_M3PSU_CHANNEL_STATUS_56]    = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 5000 },
    [CAN_MSG_ID_M3PSU_CHANNEL_STATUS_78]    = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 5000 },
    [CAN_MSG_ID_M3PSU_CHANNEL_STATUS_910]   = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 5000 },
    [CAN_MSG_ID_M3PSU_CHANNEL_STATUS_1112]  = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 5000 },
    [CAN_MSG_ID_M3PSU_PYRO_STATUS]          = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 5000 },
    [CAN_MSG_ID_M3PSU_CHARGER_STATUS]       = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 5000 },
    [CAN_MSG_ID_M3PSU_CAPACITY]             = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 5000 },
    [CAN_MSG_ID_M3PSU_AWAKE_TIME]           = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 5000 },


    /* M3FC Packets */
    [CAN_ID_M3FC | CAN_MSG_ID_VERSION]  = { .mode = M3RADIO_ROUTER_MODE_ALWAYS },
    [CAN_ID_M3FC | CAN_MSG_ID_STATUS]   = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 1000 },
    [CAN_MSG_ID_M3FC_MISSION_STATE]     = { .mode = M3RADIO_ROUTER_MODE_ALWAYS },
    [CAN_MSG_ID_M3FC_ACCEL]             = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 5000 },
    [CAN_MSG_ID_M3FC_BARO]              = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 5000 },
    [CAN_MSG_ID_M3FC_SE_T_H]            = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 1000 },
    [CAN_MSG_ID_M3FC_SE_V_A]            = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 1000 },
    [CAN_MSG_ID_M3FC_SE_VAR_H]          = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 5000 },
    [CAN_MSG_ID_M3FC_SE_VAR_V_A]        = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 5000 },
    [CAN_MSG_ID_M3FC_CFG_PROFILE]       = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 4000 },
    [CAN_MSG_ID_M3FC_CFG_PYROS]         = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 4000  },
    [CAN_MSG_ID_M3FC_CFG_ACCEL_X]       = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 4000  },
    [CAN_MSG_ID_M3FC_CFG_ACCEL_Y]       = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 4000  },
    [CAN_MSG_ID_M3FC_CFG_ACCEL_Z]       = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 4000  },
    [CAN_MSG_ID_M3FC_CFG_RADIO_FREQ]    = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 4000  },
    [CAN_MSG_ID_M3FC_CFG_CRC]           = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 4000  },


    /* M3DL Packets */
    [CAN_ID_M3DL | CAN_MSG_ID_VERSION]  = { .mode = M3RADIO_ROUTER_MODE_ALWAYS },
    [CAN_ID_M3DL | CAN_MSG_ID_STATUS]   = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 1000},
    [CAN_MSG_ID_M3DL_FREE_SPACE]        = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 5000 },
    [CAN_MSG_ID_M3DL_RATE]              = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 5000 },
    [CAN_MSG_ID_M3DL_TEMP_1_2]          = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 5000 },
    [CAN_MSG_ID_M3DL_TEMP_3_4]          = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 5000 },
    [CAN_MSG_ID_M3DL_TEMP_5_6]          = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 5000 },
    [CAN_MSG_ID_M3DL_TEMP_7_8]          = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 5000 },
    [CAN_MSG_ID_M3DL_TEMP_9]            = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 5000 },
    [CAN_MSG_ID_M3DL_PRESSURE]          = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 5000 },


    /* M3IMU Packets */
    [CAN_ID_M3IMU | CAN_MSG_ID_VERSION]  = { .mode = M3RADIO_ROUTER_MODE_ALWAYS },
    [CAN_ID_M3IMU | CAN_MSG_ID_STATUS]   = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 1000 },


    /* M3Pyro Packets */
    [CAN_ID_M3PYRO | CAN_MSG_ID_VERSION] = { .mode = M3RADIO_ROUTER_MODE_ALWAYS },
    [CAN_ID_M3PYRO | CAN_MSG_ID_STATUS]  = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 1000 },
    [CAN_MSG_ID_M3PYRO_FIRE_STATUS]      = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 1000 },
    [CAN_MSG_ID_M3PYRO_ARM_STATUS]       = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 1000 },
    [CAN_MSG_ID_M3PYRO_CONTINUITY]       = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 1000 },
    [CAN_MSG_ID_M3PYRO_SUPPLY_STATUS]    = { .mode = M3RADIO_ROUTER_MODE_TIMED, .period = 1000 },
};
