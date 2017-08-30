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

/* Timestamps */
uint32_t time_capture_pps_timestamp;
uint32_t time_capture_radio_timestamp;

/* Semaphores */
binary_semaphore_t pps_event_sem;
binary_semaphore_t radio_sync_event_sem;

/* Mutexs */
mutex_t range_pkt_mutex;

/* Ranging Packet */
ranging_packet range_pkt;


void measurements_handle_pps(void) {

    /* PPS Timestamp */
    time_capture_pps_timestamp = TIM2->CCR1;
    
    /* Signal PPS Semaphore */
	chSysLockFromISR();
	chBSemSignalI(&pps_event_sem);
	chSysUnlockFromISR();    
}


void measurements_handle_radio(void) {

    /* Radio Timestamp */
    time_capture_radio_timestamp = TIM2->CCR2;
    
    /* Signal SYNC Semaphore */
	chSysLockFromISR();
	chBSemSignalI(&radio_sync_event_sem);
	chSysUnlockFromISR();
}


/* Measurements Thread */
static THD_WORKING_AREA(waMEASUREThread, 256);
static THD_FUNCTION(MEASUREThread, arg) {

    (void)arg;
    chRegSetThreadName("Measurements");
        
    while(TRUE) {   

        /* Loop until PPS Event */
        if (chBSemWaitTimeout(&pps_event_sem, MS2ST(1000)) == MSG_TIMEOUT) {
            continue;    
        }        
                  
        /* Wait for Radio Sync Event */
        if (chBSemWaitTimeout(&radio_sync_event_sem, MS2ST(1000)) == MSG_TIMEOUT) {
            continue;
        }

        /* Lock Mutexs */
        chMtxLock(&range_pkt_mutex);
        chMtxLock(&pvt_stamp_mutex);
        chMtxLock(&psu_status_mutex);
        
        /* Populate Ranging Packet */
        range_pkt.type = (PACKET_RANGE | TOAD_ID);
        range_pkt.time_of_week = stamped_pvt.time_of_week;
        range_pkt.tof = (stamped_pvt.pps_timestamp - time_capture_radio_timestamp);
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
    
    /* Init Mutexs */
    chMtxObjectInit(&pvt_stamp_mutex);
    chMtxObjectInit(&range_pkt_mutex);
    
    /* Create Semaphores */
    chBSemObjectInit(&pps_event_sem, TRUE);
    chBSemObjectInit(&radio_sync_event_sem, TRUE);
    
    /* Create Thread */
    chThdCreateStatic(waMEASUREThread, sizeof(waMEASUREThread), NORMALPRIO, MEASUREThread, NULL);
}
