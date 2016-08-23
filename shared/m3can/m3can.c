/*
 * M3 CAN Send/Receive Wrappers
 * Cambridge University Spaceflight
 */

#include <string.h>

#include "ch.h"
#include "hal.h"
#include "m3can.h"


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


void can_send(uint16_t msg_id, bool can_rtr, uint8_t *data, uint8_t datalen){
  static CANTxFrame txmsg;
  
  chDbgAssert(datalen <= 8, "CAN packet >8 bytes");
  
  txmsg.IDE = CAN_IDE_STD;
  if(can_rtr == false){
    txmsg.RTR = CAN_RTR_DATA;
  }else{
    txmsg.RTR = CAN_RTR_REMOTE;
  }
  txmsg.DLC = datalen;
  txmsg.SID = msg_id;
  
  memcpy(&txmsg.data8, data, datalen);
  
  canTransmit(&CAND1, CAN_ANY_MAILBOX, &txmsg, MS2ST(100));
}


static THD_WORKING_AREA(can_rx_wa, 512);
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


void can_init(void){
  canStart(&CAND1, &cancfg);
  
  chThdCreateStatic(can_rx_wa, sizeof(can_rx_wa), NORMALPRIO+10,
                      can_rx_thd, NULL);
}

