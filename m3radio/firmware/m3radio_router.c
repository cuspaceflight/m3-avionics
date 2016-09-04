#include <string.h>
#include "ch.h"
#include "m3can.h"
#include "m3radio_status.h"
#include "m3radio_router_slots.h"
#include "m3radio_router.h"

#define MEMPOOL_SIZE (128)

struct pool_frame {
    uint16_t sid;
    uint8_t rtr;
    uint8_t dlc;
    uint8_t data[8];
};

static memory_pool_t m3radio_router_mempool;
static mailbox_t     m3radio_router_mailbox;
static volatile uint8_t
    m3radio_router_mempool_buf[MEMPOOL_SIZE * sizeof(struct pool_frame)]
    __attribute__((aligned(sizeof(void*))))
    __attribute__((section(".MAIN_STACK_RAM")));
static volatile msg_t m3radio_router_mailbox_buf[MEMPOOL_SIZE]
    __attribute__((section(".MAIN_STACK_RAM")));

static void m3radio_enqueue(uint16_t sid, bool rtr,
                            uint8_t* data, uint8_t dlc);

/*
 * Handles incoming CAN packet, adding it to the queue if appropriate.
 */
void m3radio_router_handle_can(uint16_t sid, bool rtr,
                               uint8_t* data, uint8_t dlc)
{
    if(sid >= 2048) {
        m3status_set_error(M3RADIO_COMPONENT_ROUTER,
                           M3RADIO_ERROR_ROUTER_BAD_MSGID);
        return;
    }

    struct m3radio_slot* slot = &m3radio_slots[sid];

    if(slot->mode == M3RADIO_ROUTER_MODE_NEVER)
    {
        return;
    } else if(slot->mode == M3RADIO_ROUTER_MODE_ALWAYS)
    {
        m3radio_enqueue(sid, rtr, data, dlc);
    } else if(slot->mode == M3RADIO_ROUTER_MODE_TIMED)
    {
        if(chVTTimeElapsedSinceX(slot->tx_time) > MS2ST(slot->period)) {
            slot->tx_time = chVTGetSystemTimeX();
            m3radio_enqueue(sid, rtr, data, dlc);
        }
    } else if(slot->mode == M3RADIO_ROUTER_MODE_COUNT)
    {
        slot->skip_count++;
        if(slot->skip_count >= slot->count) {
            slot->skip_count = 0;
            m3radio_enqueue(sid, rtr, data, dlc);
        }
    }
}

static void m3radio_enqueue(uint16_t sid, bool rtr,
                            uint8_t* data, uint8_t dlc)
{
    void* mem = chPoolAlloc(&m3radio_router_mempool);
    struct pool_frame* frame = (struct pool_frame*)mem;
    frame->sid = sid;
    frame->rtr = (uint8_t)rtr;
    frame->dlc = dlc;
    memcpy(frame->data, data, dlc);
    msg_t rv = chMBPost(&m3radio_router_mailbox, (intptr_t)mem, TIME_IMMEDIATE);
    if(rv != MSG_OK) {
        chPoolFree(&m3radio_router_mempool, (void*)mem);
    }
}

static THD_WORKING_AREA(m3radio_router_thd_wa, 512);
static THD_FUNCTION(m3radio_router_thd, arg) {
    (void)arg;
    while(true) {
        m3status_set_ok(M3RADIO_COMPONENT_ROUTER);
    }
}

void m3radio_router_init() {
    m3status_set_init(M3RADIO_COMPONENT_ROUTER);

    chMBObjectInit(&m3radio_router_mailbox,
                   (msg_t*)m3radio_router_mailbox_buf, MEMPOOL_SIZE);
    chPoolObjectInit(&m3radio_router_mempool, MEMPOOL_SIZE, NULL);
    chPoolLoadArray(&m3radio_router_mempool,
                    (void*)m3radio_router_mempool_buf, MEMPOOL_SIZE);

    chThdCreateStatic(m3radio_router_thd_wa, sizeof(m3radio_router_thd_wa),
                      NORMALPRIO, m3radio_router_thd, NULL);
}
