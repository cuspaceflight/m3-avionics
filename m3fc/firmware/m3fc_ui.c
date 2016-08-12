#include "ch.h"
#include "hal.h"
#include "m3fc_ui.h"

/*static uint8_t leds_ok_count = 0, leds_warn_count = 0, leds_err_count = 0;*/

static PWMConfig pwm_cfg = {
    .frequency = 80000,
    .period = 20,
    .callback = NULL,
    .channels = {
        {
            .mode = PWM_OUTPUT_ACTIVE_HIGH,
            .callback = NULL,
        },
        {
            .mode = PWM_OUTPUT_DISABLED,
            .callback = NULL,
        },
        {
            .mode = PWM_OUTPUT_DISABLED,
            .callback = NULL,
        },
        {
            .mode = PWM_OUTPUT_DISABLED,
            .callback = NULL,
        },
    },
};

static THD_WORKING_AREA(leds_thd_wa, 128);
static THD_FUNCTION(leds_thd, arg) {
    (void)arg;
    chRegSetThreadName("ui_leds");
    palClearLine(LINE_LED_RED);
    while(true) {
        palSetLine(LINE_LED_YLW);
        chThdSleepMilliseconds(300);
        palClearLine(LINE_LED_YLW);
        chThdSleepMilliseconds(300);
    }
}

static THD_WORKING_AREA(beeper_thd_wa, 128);
static THD_FUNCTION(beeper_thd, arg) {
    (void)arg;
    chRegSetThreadName("ui_beeper");
    pwmStart(&PWMD5, &pwm_cfg);
    while(true) {
        pwmDisableChannel(&PWMD5, 0);
        /*pwmEnableChannel(&PWMD5, 0, 10);*/
        chThdSleepMilliseconds(100);
        pwmDisableChannel(&PWMD5, 0);
        chThdSleepMilliseconds(100);
    }
}

void m3fc_ui_beep(uint8_t freq) {
    (void)freq;
}

void m3fc_ui_init() {
    chThdCreateStatic(leds_thd_wa, sizeof(leds_thd_wa),
                      NORMALPRIO, leds_thd, NULL);
    chThdCreateStatic(beeper_thd_wa, sizeof(beeper_thd_wa),
                      NORMALPRIO, beeper_thd, NULL);
}
