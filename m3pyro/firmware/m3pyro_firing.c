#include "ch.h"
#include "hal.h"
#include "m3can.h"
#include "m3pyro_arming.h"
#include "m3pyro_firing.h"
#include "m3pyro_status.h"

#define FIRE_OFF         (0)
#define FIRE_EMATCH      (1)
#define FIRE_TALON       (2)
#define FIRE_METRON      (3)
#define FIRE_QUEUE_CLEAR (4) // when fire command in channel_fire_queue has been executed and not updated

#define EMATCH_FIRE_TIME_MS     (1500)
#define TALON_FIRE_TIME_MS      (3000)
#define METRON_FIRE_TIME_MS     (500)

//Maybe contain in a struct?////////////////
static volatile uint8_t channel_fire_queue[4] = {0}; //Inputs from m3pyro_firing_fire, cleared when executed in firing thread
static volatile uint8_t channel_fire_state[4] = {0}; //Actual state of channel, used for reporting
////////////////////////////////////////////
static virtual_timer_t channel_timers[4];
static binary_semaphore_t firing_reporter_thd_sem;

static void disable_channel(void* arg) {
    int channel_state = (int)arg;
    channel_fire_queue[channel_state] = FIRE_OFF;
    channel_fire_state[channel_state] = FIRE_OFF; // for the firing reporter

    chSysLockFromISR();
    chBSemSignalI(&firing_reporter_thd_sem);
    chSysUnlockFromISR();
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
 * Sends out fire status every 300ms, enough that we should mostly see any
 * event at least once, but we also specifically send this when starting or
 * stopping firing.
 */
static THD_WORKING_AREA(m3pyro_firing_reporter_thd_wa, 128);
static THD_FUNCTION(m3pyro_firing_reporter_thd, arg) {
    (void)arg;
    while(true) {
        can_send(CAN_MSG_ID_M3PYRO_FIRE_STATUS, false,
                 (uint8_t*)channel_fire_state, sizeof(channel_fire_state));
        chBSemWaitTimeout(&firing_reporter_thd_sem, MS2ST(300));
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

    while(true) {
        int i;
        for(i=0; i<4; i++) {

            /* Only fire at all if we're armed. */
            if(!m3pyro_arming_armed() || channel_fire_queue[i] == FIRE_OFF) {
                palClearLine(fire_line[i]);
                channel_fire_queue[i] = FIRE_QUEUE_CLEAR;
                channel_fire_state[i] = FIRE_OFF;
            } else if(channel_fire_queue[i] == FIRE_EMATCH) {
                palSetLine(fire_line[i]);
                set_timer(i, EMATCH_FIRE_TIME_MS);
                channel_fire_queue[i] = FIRE_QUEUE_CLEAR;
                channel_fire_state[i] = FIRE_EMATCH;
            } else if(channel_fire_queue[i] == FIRE_TALON) {
                palSetLine(fire_line[i]);
                set_timer(i, TALON_FIRE_TIME_MS);
                channel_fire_queue[i] = FIRE_QUEUE_CLEAR;
                channel_fire_state[i] = FIRE_TALON;
            } else if(channel_fire_queue[i] == FIRE_METRON) {
                palToggleLine(fire_line[i]);
                set_timer(i, METRON_FIRE_TIME_MS);
                channel_fire_queue[i] = FIRE_QUEUE_CLEAR;
                channel_fire_state[i] = FIRE_METRON;
            } else if(channel_fire_queue[i] == FIRE_QUEUE_CLEAR 
                      && channel_fire_state[i] == FIRE_METRON){
                palToggleLine(fire_line[i]);
            }
   
        }

        m3status_set_ok(M3PYRO_COMPONENT_FIRING);

        /* 10ms gives the right toggle frequency for Metrons and is pretty fast
         * for checking everything else.
         */
        chThdSleepMilliseconds(10);
    }
}

void m3pyro_firing_init() {
    m3status_set_init(M3PYRO_COMPONENT_FIRING);

    chBSemObjectInit(&firing_reporter_thd_sem, false);

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

void m3pyro_firing_channel(uint8_t ch, uint8_t fire_state) {
    /*Ensure fire state valid, no enums?*/
    bool fire_state_valid = false;

    if (fire_state == FIRE_OFF || fire_state == FIRE_EMATCH ||
        fire_state == FIRE_TALON || fire_state == FIRE_METRON){
        fire_state_valid = true;
    }

    /* Only react if we're armed and inputs are valid*/
    if(m3pyro_arming_armed() && ch < 4 && fire_state_valid) {
        channel_fire_queue[ch] = fire_state;
    } else {
        channel_fire_queue[ch] = FIRE_OFF;
    }

    /* Send out the new firing state */
    chBSemSignal(&firing_reporter_thd_sem);
}

//Kept for backward compatibility
void m3pyro_firing_fire(uint8_t ch1, uint8_t ch2, uint8_t ch3, uint8_t ch4) {

    /* Only react if we're armed */
    if(m3pyro_arming_armed()) {
        channel_fire_queue[0] = ch1;
        channel_fire_queue[1] = ch2;
        channel_fire_queue[2] = ch3;
        channel_fire_queue[3] = ch4;
    } else {
        channel_fire_queue[0] = FIRE_OFF;
        channel_fire_queue[1] = FIRE_OFF;
        channel_fire_queue[2] = FIRE_OFF;
        channel_fire_queue[3] = FIRE_OFF;
    }

    /* Send out the new firing state */
    chBSemSignal(&firing_reporter_thd_sem);
}
