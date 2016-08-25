#include "ch.h"
#include "hal.h"
#include "m3can.h"
#include "m3pyro_arming.h"
#include "m3pyro_firing.h"
#include "m3pyro_status.h"

#define FIRE_OFF        (0)
#define FIRE_EMATCH     (1)
#define FIRE_TALON      (2)
#define FIRE_METRON     (3)

#define EMATCH_FIRE_TIME_MS     (1500)
#define TALON_FIRE_TIME_MS      (3000)
#define METRON_FIRE_TIME_MS     (500)

static volatile uint8_t channel_fire_state[4] = {0};
static virtual_timer_t channel_timers[4];

static void disable_channel(void* arg) {
    uint8_t* channel_state = (uint8_t*)arg;
    *channel_state = FIRE_OFF;
}

static void set_timer(int channel, int fire_time_ms) {
    chSysLock();
    if(chVTIsArmedI(&channel_timers[channel])) {
        chVTDoResetI(&channel_timers[channel]);
    }

    chVTDoSetI(&channel_timers[channel], MS2ST(fire_time_ms),
               disable_channel, &channel);

    chSysUnlock();
}

/* Reporting thread
 * Sends out fire status every 100ms
 */
static THD_WORKING_AREA(m3pyro_firing_reporter_thd_wa, 128);
static THD_FUNCTION(m3pyro_firing_reporter_thd, arg) {
    (void)arg;
    while(true) {
        uint8_t state[4] = {
            channel_fire_state[0], channel_fire_state[1],
            channel_fire_state[2], channel_fire_state[3]
        };
        can_send(CAN_MSG_ID_M3PYRO_FIRE_STATUS, false,
                 state, sizeof(state));
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
    ioline_t fire_line[4] = {LINE_FIRE1, LINE_FIRE2, LINE_FIRE3, LINE_FIRE4};
    uint8_t status_counter = 0;

    while(true) {
        int i;
        for(i=0; i<4; i++) {
            if(!m3pyro_arming_armed() || channel_fire_state[i] == FIRE_OFF) {
                palClearLine(fire_line[i]);
            } else if(channel_fire_state[i] == FIRE_EMATCH) {
                palSetLine(fire_line[i]);
                set_timer(i, EMATCH_FIRE_TIME_MS);
            } else if(channel_fire_state[i] == FIRE_TALON) {
                palSetLine(fire_line[i]);
                set_timer(i, TALON_FIRE_TIME_MS);
            } else if(channel_fire_state[i] == FIRE_METRON) {
                palToggleLine(fire_line[i]);
                set_timer(i, METRON_FIRE_TIME_MS);
            }
        }

        /* Send status OK every 1s */
        if(status_counter++ == 100) {
            m3status_set_ok(M3PYRO_COMPONENT_FIRING);
            status_counter = 0;
        }

        chThdSleepMilliseconds(10);
    }
}

void m3pyro_firing_init() {
    m3status_set_init(M3PYRO_COMPONENT_FIRING);

    int i;
    for(i=0; i<4; i++) {
        chVTObjectInit(&channel_timers[i]);
    }

    chThdCreateStatic(m3pyro_firing_thd_wa, sizeof(m3pyro_firing_thd_wa),
                      HIGHPRIO, m3pyro_firing_thd, NULL);
    chThdCreateStatic(m3pyro_firing_reporter_thd_wa,
                      sizeof(m3pyro_firing_reporter_thd_wa),
                      NORMALPRIO, m3pyro_firing_reporter_thd, NULL);
}

void m3pyro_firing_fire(uint8_t ch1, uint8_t ch2, uint8_t ch3, uint8_t ch4) {
    if(m3pyro_arming_armed()) {
        channel_fire_state[0] = ch1;
        channel_fire_state[1] = ch2;
        channel_fire_state[2] = ch3;
        channel_fire_state[3] = ch4;
    } else {
        channel_fire_state[0] = FIRE_OFF;
        channel_fire_state[1] = FIRE_OFF;
        channel_fire_state[2] = FIRE_OFF;
        channel_fire_state[3] = FIRE_OFF;
    }
}
