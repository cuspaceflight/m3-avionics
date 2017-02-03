#include <string.h>
#include "ch.h"
#include "hal.h"
#include "usbcfg.h"
#include "lab01_labrador.h"

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

static THD_WORKING_AREA(usbserial_thd_wa, 1024);
static THD_FUNCTION(usbserial_thd, arg)
{
    (void)arg;
    sduObjectInit(&SDU1);
    sduStart(&SDU1, &serusbcfg);
    usbDisconnectBus(serusbcfg.usbp);
    chThdSleepMilliseconds(1500);
    usbStart(serusbcfg.usbp, &usbcfg);
    usbConnectBus(serusbcfg.usbp);

    usb_setup = true;

    struct can_msg msg;
    int msgidx = 0;

    while(true) {
        uint8_t c = chSequentialStreamGet(&SDU1);
        if(c == 0x7E) {
            msgidx = 0;
            continue;
        }
        if(c == 0x7D) {
            c = chSequentialStreamGet(&SDU1) ^ 0x20;
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

void usbserial_init(void)
{
    chThdCreateStatic(usbserial_thd_wa, sizeof(usbserial_thd_wa),
                      NORMALPRIO, usbserial_thd, NULL);
}

void usbserial_send(uint16_t sid, uint8_t rtr, uint8_t* data, uint8_t dlc)
{
    uint8_t buf[16];
    struct can_msg msg = { .id = sid, .rtr = rtr, .len = dlc };
    size_t bufidx = 0;
    memcpy(msg.data, data, dlc);
    buf[bufidx++] = 0x7E;
    for(size_t i=0; i<12; i++) {
        uint8_t c = msg.raw[i];
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

    while(usb_setup != true) {
        chThdSleepMilliseconds(10);
    }

    chSequentialStreamWrite(&SDU1, buf, bufidx);
}
