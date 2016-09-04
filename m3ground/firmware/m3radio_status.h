#ifndef M3_STATUS_H
#define M3_STATUS_H

#include <stdint.h>

#define M3RADIO_COMPONENT_SI4460 0
#define M3RADIO_ERROR_SI4460_CFG 0

/* Fake functions to appease the si4460 driver */
void m3status_set_init(uint8_t component);
void m3status_set_ok(uint8_t component);
void m3status_set_error(uint8_t component, uint8_t errorcode);

#endif
