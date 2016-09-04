#include "m3radio_status.h"
#include "m3can.h"

/* Fake functions to appease the si4460 driver */
void m3status_set_init(uint8_t component) {
    (void)component;
}

void m3status_set_ok(uint8_t component) {
    (void)component;
}

void m3status_set_error(uint8_t component, uint8_t errorcode) {
    (void)component;
    (void)errorcode;
}

void can_send_u8(uint16_t msg_id, uint8_t d0, uint8_t d1, uint8_t d2,
                 uint8_t d3, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7,
                 size_t n)
{
    (void)msg_id;
    (void)d0; (void)d1; (void)d2; (void)d3;
    (void)d4; (void)d5; (void)d6; (void)d7;
    (void)n;
}
