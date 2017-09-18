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
    __attribute__((section(".ram4")));
static volatile msg_t mailbox_buf[MEMPOOL_SIZE]
    __attribute__((section(".ram4")));

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

    if(sid != (CAN_ID_M3RADIO|CAN_MSG_ID_STATUS)) {
        m3status_set_ok(M3RADIO_COMPONENT_ROUTER);
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

void m3radio_router_fillbuf(uint8_t* buf, size_t len)
{
    msg_t mailbox_rv;
    msg_t msg;
    cnt_t messages_remaining;
    size_t bufidx = 2;
    uint8_t n_frames = 0;
    struct pool_frame *frame;

    /* Check how many messages are in the mailbox for transmission */
    chSysLock();
    messages_remaining = chMBGetUsedCountI(&mailbox);
    chSysUnlock();

    /* Keep adding to the buffer until we either run out of buffer
     * or messages.
     */
    while(messages_remaining > 0) {
        /* Try and fetch a message, on error stop adding to the buffer. */
        mailbox_rv = chMBFetch(&mailbox, &msg, TIME_IMMEDIATE);
        if(mailbox_rv != MSG_OK || msg == 0) {
            break;
        }
        frame = (struct pool_frame*)msg;

        /* If packet buffer is too full to take this frame, stop here.
         * We'll post the messages back into the mailbox so it's first
         * to be transmitted next time.
         */
        size_t frame_size = 2 + frame->dlc;
        if(bufidx + frame_size >= len) {
            chMBPostAhead(&mailbox, msg, TIME_IMMEDIATE);
            break;
        }

        /* Otherwise, add this frame to the buffer. */
        buf[bufidx+0]  = (frame->sid >> 3) & 0x00FF;
        buf[bufidx+1]  = (frame->sid << 5) & 0x00E0;
        buf[bufidx+1] |= (frame->rtr << 5) & 0x0010;
        buf[bufidx+1] |= (frame->dlc     ) & 0x000F;
        memcpy(&buf[bufidx+2], frame->data, frame->dlc);
        bufidx += frame_size;
        n_frames++;

        /* Release this frame from the memory pool. */
        chPoolFree(&mempool, (void*)msg);

        /* Update number of messages remaining. */
        chSysLock();
        messages_remaining = chMBGetUsedCountI(&mailbox);
        chSysUnlock();
    }

    /* Set the first two bytes of the buffer to number of frames and
     * number of remaining enqueued packets.
     */
    buf[0] = (uint8_t)n_frames;
    buf[1] = (uint8_t)messages_remaining;
}

void m3radio_router_init() {
    m3status_set_init(M3RADIO_COMPONENT_ROUTER);

    chMBObjectInit(&mailbox, (msg_t*)mailbox_buf, MEMPOOL_SIZE);
    chPoolObjectInit(&mempool, sizeof(struct pool_frame), NULL);
    chPoolLoadArray(&mempool, (void*)mempool_buf, MEMPOOL_SIZE);
}
