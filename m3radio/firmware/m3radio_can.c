#include "m3can.h"
#include "m3radio_status.h"
#include "m3radio_router.h"

void can_recv(uint16_t msg_id, bool can_rtr, uint8_t* data, uint8_t datalen) {
    m3radio_router_handle_can(msg_id, can_rtr, data, datalen);
}
