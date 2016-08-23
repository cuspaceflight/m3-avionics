#include "ch.h"
#include "hal.h"
#include "m3can.h"
#include "m3pyro_arming.h"
#include "m3pyro_firing.h"

static uint8_t fire_ch1=0, fire_ch2=0, fire_ch3=0, fire_ch4=0;

/* Reporting thread
 * Sends out fire status every 100ms
 */
static THD_WORKING_AREA(m3pyro_firing_reporter_thd_wa, 128);
static THD_FUNCTION(m3pyro_firing_reporter_thd, arg) {
    (void)arg;
    while(true) {
        uint8_t data[4] = {fire_ch1, fire_ch2, fire_ch3, fire_ch4};
        can_send(CAN_MSG_ID_M3PYRO_FIRE_STATUS, false, data, sizeof(data));
        chThdSleepMilliseconds(100);
    }
}

/*
 * Firing thread
 * Every 10ms it updates all fire line statuses, either off, on, or toggling.
 */
static THD_WORKING_AREA(m3pyro_firing_thd_wa, 256);
static THD_FUNCTION(m3pyro_firing_thd, arg) {
    (void)arg;

    while(true) {
        uint8_t fire_state[4] = {fire_ch1, fire_ch2, fire_ch3, fire_ch4};
        ioline_t fire_line[4] = {LINE_FIRE1, LINE_FIRE2, LINE_FIRE3, LINE_FIRE4};
        int i;
        for(i=0; i<4; i++) {
            if(!m3pyro_arming_armed()) {
                palClearLine(fire_line[i]);
            } else if(fire_state[i] == 0) {
                palClearLine(fire_line[i]);
            } else if(fire_state[i] == 1) {
                palSetLine(fire_line[i]);
            } else if(fire_state[i] == 2) {
                palToggleLine(fire_line[i]);
            }
        }
        chThdSleepMilliseconds(10);
    }
}

void m3pyro_firing_init() {
    chThdCreateStatic(m3pyro_firing_thd_wa, sizeof(m3pyro_firing_thd_wa),
                      HIGHPRIO, m3pyro_firing_thd, NULL);
    chThdCreateStatic(m3pyro_firing_reporter_thd_wa,
                      sizeof(m3pyro_firing_reporter_thd_wa),
                      NORMALPRIO, m3pyro_firing_reporter_thd, NULL);
}

void m3pyro_firing_fire(uint8_t ch1, uint8_t ch2, uint8_t ch3, uint8_t ch4) {
    if(m3pyro_arming_armed()) {
        fire_ch1 = ch1;
        fire_ch2 = ch2;
        fire_ch3 = ch3;
        fire_ch4 = ch4;
    }
}
