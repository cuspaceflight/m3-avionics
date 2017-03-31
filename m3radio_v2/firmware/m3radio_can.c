#include "m3can.h"
#include "m3radio_status.h"
#include "m3radio_router.h"

void m3can_recv(uint16_t msg_id, bool rtr, uint8_t* data, uint8_t datalen) {
    m3radio_router_handle_can(msg_id, rtr, data, datalen);
}
