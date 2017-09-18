#include <string.h>
#include "ch.h"
#include "hal.h"
#include "usbcfg.h"
#include "lab01_labrador.h"

#define MEMPOOL_SIZE (64)

static volatile bool usb_setup = false;

struct can_msg {
    union {
        struct {
            uint16_t id;
            uint8_t  rtr;
            uint8_t  len;
            uint8_t  data[8];
        };
        uint8_t raw[12];
    };
};

static memory_pool_t mempool;
static mailbox_t mailbox;
static volatile uint8_t mempool_buf[MEMPOOL_SIZE * sizeof(struct can_msg)]
    __attribute__((aligned(sizeof(void *))));
static volatile msg_t mailbox_buf[MEMPOOL_SIZE];


static THD_WORKING_AREA(usbserial_rx_thd_wa, 1024);
static THD_FUNCTION(usbserial_rx_thd, arg)
{
    (void)arg;
    sduObjectInit(&SDU1);
    sduStart(&SDU1, &serusbcfg);
    usbDisconnectBus(serusbcfg.usbp);
    chThdSleepMilliseconds(1500);
    usbStart(serusbcfg.usbp, &usbcfg);
    usbConnectBus(serusbcfg.usbp);
    chThdSleepMilliseconds(1000);

    usb_setup = true;

    struct can_msg msg;
    int msgidx = 0;

    while(true) {
        uint8_t c = chnGetTimeout(&SDU1, TIME_INFINITE);
        if(c == 0x7E) {
            msgidx = 0;
            continue;
        }
        if(c == 0x7D) {
            c = chnGetTimeout(&SDU1, TIME_INFINITE) ^ 0x20;
        }
        msg.raw[msgidx++] = c;
        if(msgidx >= 4 && msgidx == msg.len + 4 && msg.len <= 8) {
            lab01_labrador_send(msg.id, msg.rtr, msg.data, msg.len);
            msgidx = 0;
        }
        if(msgidx >= 12) {
            msgidx = 0;
        }
    }
}

static THD_WORKING_AREA(usbserial_tx_thd_wa, 1024);
static THD_FUNCTION(usbserial_tx_thd, arg)
{
    (void)arg;
    while(!usb_setup) {
        chThdSleepMilliseconds(100);
    }

    struct can_msg *frame;
    uint8_t buf[16];
    size_t bufidx;

    while(true) {
        msg_t rv = chMBFetch(&mailbox, (msg_t*)&frame, MS2ST(100));
        if(rv != MSG_OK || frame == 0) continue;
        bufidx = 0;
        buf[bufidx++] = 0x7E;
        for(size_t i=0; i<12; i++) {
            uint8_t c = frame->raw[i];
            if(c == 0x7E) {
                buf[bufidx++] = 0x7D;
                buf[bufidx++] = 0x5E;
            } else if(c == 0x7D) {
                buf[bufidx++] = 0x7D;
                buf[bufidx++] = 0x5D;
            } else {
                buf[bufidx++] = c;
            }
        }
        chnWrite(&SDU1, buf, bufidx);
        chPoolFree(&mempool, (void*)frame);
    }
}

void usbserial_init(void)
{
    chMBObjectInit(&mailbox, (msg_t*)mailbox_buf, MEMPOOL_SIZE);
    chPoolObjectInit(&mempool, sizeof(struct can_msg), NULL);
    chPoolLoadArray(&mempool, (void*)mempool_buf, MEMPOOL_SIZE);

    chThdCreateStatic(usbserial_rx_thd_wa, sizeof(usbserial_rx_thd_wa),
                      NORMALPRIO, usbserial_rx_thd, NULL);
    chThdCreateStatic(usbserial_tx_thd_wa, sizeof(usbserial_tx_thd_wa),
                      NORMALPRIO, usbserial_tx_thd, NULL);
}

void usbserial_send(uint16_t sid, uint8_t rtr, uint8_t* data, uint8_t dlc)
{
    struct can_msg *msg;
    msg = (struct can_msg*)chPoolAlloc(&mempool);
    msg->id = sid;
    msg->rtr = rtr;
    msg->len = dlc;
    memcpy(msg->data, data, dlc);
    msg_t rv = chMBPost(&mailbox, (intptr_t)msg, TIME_IMMEDIATE);
    if(rv != MSG_OK) {
        chPoolFree(&mempool, (void*)msg);
    }
}
