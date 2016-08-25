#ifndef M3_STATUS_H
#define M3_STATUS_H

#include <stdint.h>

#define M3STATUS_OK                 (0)
#define M3STATUS_INITIALISING       (1<<0)
#define M3STATUS_ERROR              (1<<1)

/* Call to update status, with optional error code.
 * Call initialising() for each component to start including that component ID
 * and to send an initialising status message for it.
 * Call ok() on components once they finish initialising successfully.
 * Call error() on components whenever an error occurs, with an optional error
 * code to give more details.
 * Every call, a status packet is sent for that specific component which
 * also includes the AND of every component's OK status, to give an overall
 * status for this board.
 */
void m3status_set_init(uint8_t component);
void m3status_set_ok(uint8_t component);
void m3status_set_error(uint8_t component, uint8_t errorcode);

/* Get the current board's overall status */
uint8_t m3status_get(void);

/* Get an individual component's current status */
uint8_t m3status_get_component(uint8_t component);

#endif
