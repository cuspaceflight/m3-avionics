#include "ch.h"
#include "hal.h"

#include "timer.h"
#include "measurements.h"
#include "status.h"
#include "gps.h"
#include "packets.h"
#include "config.h"
#include "psu.h"
#include "logging.h"

MUTEX_DECL(range_pkt_mutex);
BSEMAPHORE_DECL(pps_event_sem, TRUE);
BSEMAPHORE_DECL(radio_sync_event_sem, TRUE);

/* Timestamps */
uint32_t time_capture_pps_timestamp;
uint32_t time_capture_radio_timestamp;

static int32_t rxclk_deltas[1200];
static int32_t rxclk_delta_acc = 0;
static uint32_t rxclk_cnt = 0;

/* Semaphores */
binary_semaphore_t pps_event_sem;
binary_semaphore_t radio_sync_event_sem;

/* Mutexs */
mutex_t range_pkt_mutex;

/* Ranging Packet */
ranging_packet range_pkt;

/* Telemetry Activity Flag */
bool telem_activity;

void measurements_start_rxclk(void)
{
    rxclk_cnt = 0;
    rxclk_delta_acc = 0;
}

/* PPS Callback */
void measurements_handle_pps(void) {

    chSysLockFromISR();

    /* PPS Timestamp */
    time_capture_pps_timestamp = TIM2->CCR1;

    /* Signal PPS Semaphore */
    chBSemSignalI(&pps_event_sem);

    chSysUnlockFromISR();

    set_status(COMPONENT_GPS, STATUS_ACTIVITY);
}


/* RADIO-SYNC Callback */
void measurements_handle_radio(void) {

    /* skip doing anything if we've already seen 1200 measurements and
     * not restarted yet */
    if(rxclk_cnt >= 1200) return;

    /* Record this clock edge.
     * We care about the time difference between its arrival and the
     * most recent PPS, which we first measure in timer counts (at 86MHz).
     * Then we want to quantise this to the time since the most recent
     * 0.5ms past the top of the second. 0.5ms is 42000 timer counts.
     * We record any differences that are more than 0.25ms as negative
     * and belonging to the previous edge. We then accumulate the reading
     * so we can later take the average.
     */
    uint32_t delta = TIM2->CCR2 - time_capture_pps_timestamp;
    int32_t mod_delta = delta % 42000;
    if(mod_delta > 21000) {
        mod_delta = 42000 - mod_delta;
    }
    rxclk_deltas[rxclk_cnt] = mod_delta;
    rxclk_delta_acc += mod_delta;
    rxclk_cnt++;

    /* don't do further processing until ew have 1200 clocks */
    if(rxclk_cnt++ < 1200) return;

    /* disable rxclk until we next get a sync word */
    gpt2_disable_ccr2();

    chSysLockFromISR();

    /* average the deltas */
    time_capture_radio_timestamp = rxclk_delta_acc / 1200;

    /* Telemetry Activity */
    telem_activity = true;

    /* Signal SYNC Semaphore */
    chBSemSignalI(&radio_sync_event_sem);

    chSysUnlockFromISR();
}


/* Measurements Thread */
static THD_WORKING_AREA(waMEASUREThread, 256);
static THD_FUNCTION(MEASUREThread, arg) {

    (void)arg;
    chRegSetThreadName("Measurements");

    while(TRUE) {


        /* Wait for Radio Sync Event */
        if (chBSemWaitTimeout(&radio_sync_event_sem, MS2ST(1000)) == MSG_TIMEOUT) {

            /* No Telemetry Detected */
            telem_activity = FALSE;
            continue;
        }

        /* Lock Mutexs */
        chMtxLock(&range_pkt_mutex);
        chMtxLock(&pvt_stamp_mutex);
        chMtxLock(&psu_status_mutex);

        /* Populate Ranging Packet */
        range_pkt.type = (PACKET_RANGE | toad.id);
        range_pkt.time_of_week = stamped_pvt.time_of_week;
        range_pkt.tof = (time_capture_radio_timestamp - stamped_pvt.pps_timestamp);
        range_pkt.bat_volt = (uint8_t)(battery.voltage / 100);
        range_pkt.temp = battery.stm_temp;

        log_ranging_packet(&range_pkt);

        /* Unlock Mutexs */
        chMtxUnlock(&psu_status_mutex);
        chMtxUnlock(&pvt_stamp_mutex);
        chMtxUnlock(&range_pkt_mutex);
    }
}


/* Init Measurements */
void measurement_init(void) {

    /* Create Thread */
    chThdCreateStatic(waMEASUREThread, sizeof(waMEASUREThread), NORMALPRIO, MEASUREThread, NULL);
}
