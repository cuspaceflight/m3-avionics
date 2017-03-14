#ifndef M3RADIO_ROUTER_SLOTS_H
#define M3RADIO_ROUTER_SLOTS_H

#include "ch.h"
#include "m3can.h"
#include "m3radio_router.h"

struct m3radio_slot {
    union {
        systime_t tx_time;
        uint32_t skip_count;
    };
    union {
        uint16_t period;
        uint16_t count;
    };
    uint8_t mode;
};

extern struct m3radio_slot m3radio_slots[2048];

#endif
