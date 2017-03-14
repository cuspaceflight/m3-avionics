#ifndef M3_STATUS_H
#define M3_STATUS_H

#include <stdint.h>

#define M3STATUS_OK                 (0)
#define M3STATUS_INITIALISING       (1<<0)
#define M3STATUS_ERROR              (1<<1)

#define M3STATUS_COMPONENT_DCDC1    (0)
#define M3STATUS_COMPONENT_DCDC2    (1)
#define M3STATUS_COMPONENT_DCDC3    (2)
#define M3STATUS_COMPONENT_DCDC4    (3)
#define M3STATUS_COMPONENT_DCDC5    (4)
#define M3STATUS_COMPONENT_DCDC6    (5)
#define M3STATUS_COMPONENT_CHARGER  (6)
#define M3STATUS_COMPONENT_PYRO_MON (7)

#define M3STATUS_DCDC_ERROR_INIT      (1)
#define M3STATUS_DCDC_ERROR_CH1_ALERT (2)
#define M3STATUS_DCDC_ERROR_CH2_ALERT (3)
#define M3STATUS_DCDC_ERROR_COMMS     (4)

#define M3STATUS_CHARGER_ERROR_INIT   (1)
#define M3STATUS_CHARGER_ERROR_READ   (2)

#define M3STATUS_PYRO_MON_ERROR_INIT  (1)
#define M3STATUS_PYRO_MON_ERROR_COMMS (2)

/* Call to update status, with optional error code.
 * Call initialising() for each component to start including that component ID
 * and to send an initialising status message for it.
 * Call ok() on components once they finish initialising successfully.
 * Call error() on components whenever an error occurs, with an optional error
 * code to give more details.
 * On each call, if that component's status has changed or if it has been too
 * long since we last transmitted this component's status, a new CAN status
 * packet is sent out with this update and the overall status summary.
 */
void m3status_set_init(uint8_t component);
void m3status_set_ok(uint8_t component);
void m3status_set_error(uint8_t component, uint8_t errorcode);

/* Get the current board's overall status */
uint8_t m3status_get(void);

/* Get an individual component's current status */
uint8_t m3status_get_component(uint8_t component);

#endif
