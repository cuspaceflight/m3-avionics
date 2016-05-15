#include "ch.h"
#include "hal.h"
#include "led.h"

#define OFF false
#define ON  true

volatile bool error_states[ERROR_MAX];

static void blink(int n, int ontime, int offtime)
{
#if !NO_BEEPING
    int i;
    for(i=0; i<n; i++) {
        palSetPad(GPIOA, GPIOA_BUZZER);
        chThdSleepMilliseconds(ontime);
        palClearPad(GPIOA, GPIOA_BUZZER);
        chThdSleepMilliseconds(offtime);
    }
#else
    chThdSleepMilliseconds(n*(ontime + offtime));
#endif
}

msg_t led_thread(void* arg)
{
    (void)arg;
    chRegSetThreadName("LED");
    int i;
    for(i=0; i<ERROR_MAX; i++) {
        error_states[i] = false;
    }

    while(true) {
        bool all_ok = true;
        for(i=0; i<ERROR_MAX; i++) {
            if(error_states[i]) {
                all_ok = false;
                blink(i, 500, 200);
                chThdSleepMilliseconds(1000);
            }
        }
        if(all_ok) {
            blink(1, 10, 990);
        }
    }
}

void led_set_error(led_error_t err, bool set)
{
    error_states[err] = set;
}
