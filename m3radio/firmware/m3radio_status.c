#include "m3radio_status.h"
#include "hal.h"
#include "ch.h"

static THD_WORKING_AREA(m3radio_status_thd_wa, 128);
static THD_FUNCTION(m3radio_status_thd, arg) {
    (void)arg;
    while(true) {
        int status = m3status_get();
        if(status == M3STATUS_OK) {
            palSetLine(LINE_LED_GRN);
            chThdSleepMilliseconds(100);
            palClearLine(LINE_LED_GRN);
            chThdSleepMilliseconds(400);
        } else if(status == M3STATUS_INITIALISING) {
            palSetLine(LINE_LED_GRN);
            palSetLine(LINE_LED_RED);
            chThdSleepMilliseconds(100);
        } else {
            palSetLine(LINE_LED_RED);
            chThdSleepMilliseconds(200);
            palClearLine(LINE_LED_RED);
            chThdSleepMilliseconds(200);
        }
    }
}

void m3radio_status_init() {
    chThdCreateStatic(m3radio_status_thd_wa, sizeof(m3radio_status_thd_wa),
                      NORMALPRIO, m3radio_status_thd, NULL);
}
