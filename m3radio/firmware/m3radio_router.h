#ifndef M3RADIO_ROUTER_H
#define M3RADIO_ROUTER_H
#include <stdbool.h>
#include <stdint.h>

#define M3RADIO_ROUTER_MODE_NEVER  (0)
#define M3RADIO_ROUTER_MODE_ALWAYS (1)
#define M3RADIO_ROUTER_MODE_TIMED  (2)
#define M3RADIO_ROUTER_MODE_COUNT  (3)

void m3radio_router_init(void);
void m3radio_router_handle_can(uint16_t msg_id, bool rtr,
                               uint8_t* data, uint8_t datalen);

#endif
