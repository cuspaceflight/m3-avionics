/*
 * M3 CAN Send/Receive Wrappers
 * Cambridge University Spaceflight
 */

#include <string.h>

#include "ch.h"
#include "hal.h"
#include "m3can.h"

#ifndef FIRMWARE_VERSION
#error "Please check your Makefile sets FIRMWARE_VERSION"
#endif

uint8_t m3can_own_id = 0;

static volatile bool can_loopback_enabled;


static const CANConfig cancfg = {
    .mcr =
        /* Automatic Bus Off Management enabled,
         * Automatic Wake Up Management enabled.
         */
        CAN_MCR_ABOM | CAN_MCR_AWUM,
    .btr =
        /* CAN is on APB1 at 42MHz, we want 1Mbit/s.
         * 1/Baud = (BRP+1)/(APB1) * (3+TS1+TS2)
         */
        CAN_BTR_BRP(5) | CAN_BTR_TS1(3) | CAN_BTR_TS2(1)
        /* Allow up to 2 time quanta clock synchronisation */
        | CAN_BTR_SJW(1)
};


void can_send(uint16_t msg_id, bool can_rtr, uint8_t *data, uint8_t datalen) {
    static CANTxFrame txmsg;

    chDbgAssert(datalen <= 8, "CAN packet >8 bytes");

    if(can_rtr == false) {
        txmsg.RTR = CAN_RTR_DATA;
    } else {
        txmsg.RTR = CAN_RTR_REMOTE;
    }
    txmsg.IDE = CAN_IDE_STD;
    txmsg.DLC = datalen;
    txmsg.SID = msg_id;

    memcpy(&txmsg.data8, data, datalen);

    canTransmit(&CAND1, CAN_ANY_MAILBOX, &txmsg, MS2ST(100));

    if(can_loopback_enabled) {
        can_recv(msg_id, can_rtr, data, datalen);
    }
}


void can_send_u8(uint16_t msg_id, uint8_t d0, uint8_t d1, uint8_t d2,
                 uint8_t d3, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7,
                 size_t n)
{
    uint8_t buf[8] = {d0, d1, d2, d3, d4, d5, d6, d7};
    can_send(msg_id, false, buf, n);
}

void can_send_u16(uint16_t msg_id, uint16_t d0, uint16_t d1, uint16_t d2,
                  uint16_t d3, size_t n)
{
    uint8_t buf[8] = {d0, d0>>8, d1, d1>>8, d2, d2>>8, d3, d3>>8};
    can_send(msg_id, false, buf, n<<1);
}

void can_send_u32(uint16_t msg_id, uint32_t d0, uint32_t d1, size_t n)
{
    uint8_t buf[8] = {d0, d0>>8, d0>>16, d0>>24, d1, d1>>8, d1>>16, d1>>24};
    can_send(msg_id, false, buf, n<<2);
}

void can_send_i8(int16_t msg_id, int8_t d0, int8_t d1, int8_t d2,
                 int8_t d3, int8_t d4, int8_t d5, int8_t d6, int8_t d7,
                 size_t n)
{
    uint8_t buf[8] = {d0, d1, d2, d3, d4, d5, d6, d7};
    can_send(msg_id, false, buf, n);
}

void can_send_i16(int16_t msg_id, int16_t d0, int16_t d1, int16_t d2,
                  int16_t d3, size_t n)
{
    uint8_t buf[8] = {d0, d0>>8, d1, d1>>8, d2, d2>>8, d3, d3>>8};
    can_send(msg_id, false, buf, n<<1);
}

void can_send_i32(int16_t msg_id, int32_t d0, int32_t d1, size_t n)
{
    uint8_t buf[8] = {d0, d0>>8, d0>>16, d0>>24, d1, d1>>8, d1>>16, d1>>24};
    can_send(msg_id, false, buf, n<<2);
}

void can_send_f32(uint16_t msg_id, float d0, float d1, size_t n)
{
    float buf[2] = {d0, d1};
    can_send(msg_id, false, (uint8_t*)buf, n<<2);
}


static THD_WORKING_AREA(can_rx_wa, 256);
static THD_FUNCTION(can_rx_thd, arg) {
    (void)arg;

    event_listener_t el;
    CANRxFrame rxmsg;

    chEvtRegister(&CAND1.rxfull_event, &el, 0);

    while(true) {
        if(chEvtWaitAnyTimeout(ALL_EVENTS, MS2ST(100)) == 0) {
            continue;
        }

        while(canReceive(&CAND1, CAN_ANY_MAILBOX, &rxmsg,
                         TIME_IMMEDIATE) == MSG_OK) {
            can_recv(rxmsg.SID, rxmsg.RTR, rxmsg.data8, rxmsg.DLC);
        }
    }
}

void can_set_loopback(bool enabled) {
    can_loopback_enabled = enabled;
}


void can_init(uint8_t board_id) {
    m3can_own_id = board_id;
    canStart(&CAND1, &cancfg);
    can_send(board_id | CAN_MSG_ID_VERSION, false,
             (uint8_t*)FIRMWARE_VERSION, 8);
    chThdCreateStatic(can_rx_wa, sizeof(can_rx_wa), NORMALPRIO,
                      can_rx_thd, NULL);
}

