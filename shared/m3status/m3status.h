#ifndef M3_STATUS_H
#define M3_STATUS_H

#include <stdint.h>

#define M3STATUS_OK                 0
#define M3STATUS_INITIALISING       1
#define M3STATUS_ERROR              2

/* Call to update status, with optional error code */
void m3status_set_ok(void);
void m3status_set_initialising(void);
void m3status_set_error(uint8_t errorcode);

#endif
