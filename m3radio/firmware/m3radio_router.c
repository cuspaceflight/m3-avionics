#include <string.h>
#include "ch.h"
#include "m3can.h"
#include "m3radio_status.h"
#include "m3radio_router_slots.h"
#include "m3radio_router.h"
#include "m3radio_labrador.h"

#define MEMPOOL_SIZE (128)

/* We'll store instances of this struct in the memory pool */
struct pool_frame {
    uint16_t sid;
    uint8_t rtr;
    uint8_t dlc;
    uint8_t data[8];
};

/* Storage for the mempool and mailbox */
static memory_pool_t mempool;
static mailbox_t     mailbox;
static volatile uint8_t mempool_buf[MEMPOOL_SIZE * sizeof(struct pool_frame)]
    __attribute__((aligned(sizeof(void*))))
    __attribute__((section(".MAIN_STACK_RAM")));
static volatile msg_t mailbox_buf[MEMPOOL_SIZE]
    __attribute__((section(".MAIN_STACK_RAM")));

/* Storage for a single packet buffer */
static uint8_t packet_buf[M3RADIO_LABRADOR_TXBUFSIZE];

/* Send a frame into the mailbox/mempool */
static void enqueue(uint16_t sid, bool rtr, uint8_t* data, uint8_t dlc);

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
        enqueue(sid, rtr, data, dlc);
    } else if(slot->mode == M3RADIO_ROUTER_MODE_TIMED)
    {
        if(chVTTimeElapsedSinceX(slot->tx_time) > MS2ST(slot->period)) {
            slot->tx_time = chVTGetSystemTimeX();
            enqueue(sid, rtr, data, dlc);
        }
    } else if(slot->mode == M3RADIO_ROUTER_MODE_COUNT)
    {
        slot->skip_count++;
        if(slot->skip_count >= slot->count) {
            slot->skip_count = 0;
            enqueue(sid, rtr, data, dlc);
        }
    }
}

static void enqueue(uint16_t sid, bool rtr, uint8_t* data, uint8_t dlc)
{
    void* mem = chPoolAlloc(&mempool);
    struct pool_frame* frame = (struct pool_frame*)mem;
    frame->sid = sid;
    frame->rtr = (uint8_t)rtr;
    frame->dlc = dlc;
    memcpy(frame->data, data, dlc);
    msg_t rv = chMBPost(&mailbox, (intptr_t)mem, TIME_IMMEDIATE);
    if(rv != MSG_OK) {
        chPoolFree(&mempool, (void*)mem);
    }
}

static THD_WORKING_AREA(m3radio_router_thd_wa, 512);
static THD_FUNCTION(m3radio_router_thd, arg) {
    (void)arg;

    msg_t mailbox_res;
    msg_t msg;
    struct pool_frame *frame;
    uint8_t *bufptr = &packet_buf[1];
    uint8_t n_frames = 0;

    while(true) {
        /* Block trying to get a message from the mailbox */
        mailbox_res = chMBFetch(&mailbox, &msg, TIME_INFINITE);
        if(mailbox_res != MSG_OK || msg == 0) continue;
        frame = (struct pool_frame*)msg;

        /* If packet buffer is too full to take this frame,
         * send current packet buffer to radio.
         */
        size_t frame_size = 2 + frame->dlc;
        if(bufptr + frame_size > packet_buf + sizeof(packet_buf)) {
            packet_buf[0] = n_frames;
            m3radio_labrador_tx(packet_buf);
            m3status_set_ok(M3RADIO_COMPONENT_ROUTER);
            bufptr = &packet_buf[1];
            n_frames = 0;
        }

        /* Accumulate frame onto packet buffer */
        bufptr[0]  = (frame->sid >> 3) & 0x00FF;
        bufptr[1]  = (frame->sid << 5) & 0x00E0;
        bufptr[1] |= (frame->rtr << 5) & 0x0010;
        bufptr[1] |= (frame->dlc     ) & 0x000F;
        memcpy(&bufptr[2], frame->data, frame->dlc);
        bufptr += frame_size;
        n_frames++;

        /* Free the frame from the pool */
        chPoolFree(&mempool, (void*)msg);
    }
}

void m3radio_router_init() {
    m3status_set_init(M3RADIO_COMPONENT_ROUTER);

    chMBObjectInit(&mailbox, (msg_t*)mailbox_buf, MEMPOOL_SIZE);
    chPoolObjectInit(&mempool, sizeof(struct pool_frame), NULL);
    chPoolLoadArray(&mempool, (void*)mempool_buf, MEMPOOL_SIZE);

    chThdCreateStatic(m3radio_router_thd_wa, sizeof(m3radio_router_thd_wa),
                      NORMALPRIO, m3radio_router_thd, NULL);
}
