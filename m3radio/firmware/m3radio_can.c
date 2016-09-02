#include "m3can.h"
#include "m3radio_status.h"
#include "si4460.h"

void can_recv(uint16_t msg_id, bool can_rtr, uint8_t* data, uint8_t datalen) {
    (void)can_rtr;
    (void)data;
    (void)datalen;
    if(msg_id == CAN_MSG_ID_M3RADIO_GPS_LATLNG ||
       msg_id == CAN_MSG_ID_M3RADIO_GPS_ALT    ||
       msg_id == CAN_MSG_ID_M3FC_MISSION_STATE ||
       msg_id == CAN_MSG_ID_M3PYRO_CONTINUITY  ||
       msg_id == CAN_MSG_ID_M3PYRO_FIRE_COMMAND
    ) {
        /*si4460_tx_buf[0] = msg_id >> 8;*/
        /*si4460_tx_buf[1] = msg_id & 0xFF;*/
        /*si4460_tx_buf[2] = can_rtr;*/
        /*si4460_tx_buf[3] = datalen;*/
        /*int i;*/
        /*for(i=4; i<4+datalen; i++) {*/
            /*si4460_tx_buf[i] = data[i-4];*/
        /*}*/
        /*chBSemSignal(&si4460_tx_sem);*/
    }
}
