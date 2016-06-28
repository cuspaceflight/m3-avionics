
/*
 * LED error report. Reports state over the led lights.
 */
#ifndef LED_H
#define LED_H

#include <stdbool.h>
#include "ch.h"

typedef enum {
    ERROR_GPS = 1,
    ERROR_CONFIG,
    ERROR_RADIO,
    ERROR_CANBUS,
    ERROR_MAX
} led_error_t;

extern volatile bool error_states[ERROR_MAX];

/* Set a specific error.
 * If `set` is true then the error is occurring.
 */
void led_set_error(led_error_t err, bool set);
msg_t led_thread(void* arg);

#endif /* LED_H */
