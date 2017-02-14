#include "ch.h"
#include "hal.h"
#include "m3fc_ui.h"
#include "m3fc_status.h"
#include "m3fc_mission.h"

enum m3fc_ui_beeper_mode m3fc_ui_beeper_mode = M3FC_UI_BEEPER_SLOW;

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
        uint8_t status = m3status_get();
        if(status == M3STATUS_OK) {
            palSetLine(LINE_LED_GRN);
        } else if(status == M3STATUS_INITIALISING) {
            palSetLine(LINE_LED_YLW);
        } else if(status == M3STATUS_ERROR) {
            palSetLine(LINE_LED_RED);
        }
        chThdSleepMilliseconds(300);
        palClearLine(LINE_LED_GRN);
        palClearLine(LINE_LED_YLW);
        palClearLine(LINE_LED_RED);
        chThdSleepMilliseconds(300);
        m3status_set_ok(M3FC_COMPONENT_UI_LEDS);
    }
}

static THD_WORKING_AREA(beeper_thd_wa, 128);
static THD_FUNCTION(beeper_thd, arg) {
    (void)arg;
    int delay = 0;
    chRegSetThreadName("ui_beeper");
    pwmStart(&PWMD5, &pwm_cfg);
    while(true) {
        if(m3fc_ui_beeper_mode != M3FC_UI_BEEPER_OFF) {
            pwmEnableChannel(&PWMD5, 0, 10);
            chThdSleepMilliseconds(100);
            pwmDisableChannel(&PWMD5, 0);
        }

        if(m3fc_ui_beeper_mode == M3FC_UI_BEEPER_SLOW) {
            chThdSleepMilliseconds(700);
        } else {
            chThdSleepMilliseconds(100);
        }
        m3status_set_ok(M3FC_COMPONENT_UI_BEEPER);
    }
}

void m3fc_ui_init() {
    m3status_set_init(M3FC_COMPONENT_UI_BEEPER);
    m3status_set_init(M3FC_COMPONENT_UI_LEDS);
    chThdCreateStatic(leds_thd_wa, sizeof(leds_thd_wa),
                      NORMALPRIO, leds_thd, NULL);
    chThdCreateStatic(beeper_thd_wa, sizeof(beeper_thd_wa),
                      NORMALPRIO, beeper_thd, NULL);
}
