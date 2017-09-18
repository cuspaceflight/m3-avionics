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

/* Fills `buf` with queued packets for transmission,
 * up to a maximum of `len` bytes.
 *
 * Sets the first byte to the number of packets, and the second byte to
 * the number of packets that remain enqueued for later transmission,
 * then fills in remaining bytes with packets, which are a two byte header
 * followed by variable data, with the data length stored in the lowest
 * nibble of the second header byte.
 */
void m3radio_router_fillbuf(uint8_t* buf, size_t len);

#endif
