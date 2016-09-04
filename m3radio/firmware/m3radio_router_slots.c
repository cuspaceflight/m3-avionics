#include "m3radio_router_slots.h"

struct m3radio_slot m3radio_slots[2048] = {
    {.mode=M3RADIO_ROUTER_MODE_NEVER},

    [CAN_MSG_ID_M3FC_MISSION_STATE] = { .mode = M3RADIO_ROUTER_MODE_ALWAYS },
    [CAN_MSG_ID_M3FC_ACCEL]         = { .mode = M3RADIO_ROUTER_MODE_TIMED,
                                        .period = 1000 },
    [CAN_MSG_ID_M3FC_BARO]          = { .mode = M3RADIO_ROUTER_MODE_TIMED,
                                        .period = 1000 },
};
