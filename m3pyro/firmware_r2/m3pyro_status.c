#include "ch.h"
#include "hal.h"
#include "m3can.h"
#include "m3pyro_status.h"
#include "m3pyro_hal.h"

static volatile bool m3pyro_armed_flag = false;

bool m3pyro_armed(void) {
    return m3pyro_armed_flag;
}

void m3pyro_arm(void) {
    m3pyro_1a_disable();
    m3pyro_3a_disable();
    m3pyro_deassert_ch();
    m3pyro_armed_flag = true;
}

void m3pyro_disarm(void) {
    m3pyro_armed_flag = false;
    m3pyro_deassert_ch();
    m3pyro_1a_disable();
    m3pyro_3a_disable();
}

static THD_WORKING_AREA(status_leds_thd_wa, 128);
static THD_FUNCTION(status_leds_thd, arg) {
    (void)arg;
    chRegSetThreadName("status_leds");
    palClearLine(LINE_LED_RED);
    palClearLine(LINE_LED_GRN);

    while(true) {
        uint8_t status = m3status_get();
        if(status == M3STATUS_OK) {
            palSetLine(LINE_LED_GRN);
        } else if(status == M3STATUS_INITIALISING) {
            palSetLine(LINE_LED_GRN);
            palSetLine(LINE_LED_RED);
        } else if(status == M3STATUS_ERROR) {
            palSetLine(LINE_LED_RED);
        }
        chThdSleepMilliseconds(300);
        palClearLine(LINE_LED_GRN);
        palClearLine(LINE_LED_RED);
        chThdSleepMilliseconds(300);
    }
}

static THD_WORKING_AREA(armed_led_thd_wa, 128);
static THD_FUNCTION(armed_led_thd, arg) {
    (void)arg;
    chRegSetThreadName("armed_led");
    palClearLine(LINE_LED_YLW);

    while(true) {
        if(m3pyro_armed()) {
            palSetLine(LINE_LED_YLW);
        }
        chThdSleepMilliseconds(100);
        palClearLine(LINE_LED_YLW);
        chThdSleepMilliseconds(100);
    }
}

static THD_WORKING_AREA(armed_can_thd_wa, 128);
static THD_FUNCTION(armed_can_thd, arg) {
    (void)arg;
    chRegSetThreadName("armed_can");
    while(true) {
        uint8_t armed = m3pyro_armed();
        m3can_send(CAN_MSG_ID_M3PYRO_ARM_STATUS, false,
                   (uint8_t*)&armed, 1);
        chThdSleepMilliseconds(500);
    }
}

void m3pyro_status_init() {
    chThdCreateStatic(armed_can_thd_wa, sizeof(armed_can_thd_wa),
                      NORMALPRIO, armed_can_thd, NULL);
    chThdCreateStatic(status_leds_thd_wa, sizeof(status_leds_thd_wa),
                      LOWPRIO, status_leds_thd, NULL);
    chThdCreateStatic(armed_led_thd_wa, sizeof(armed_led_thd_wa),
                      LOWPRIO, armed_led_thd, NULL);
}
