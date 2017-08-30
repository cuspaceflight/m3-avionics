#include "ch.h"
#include "hal.h"

#include "timer.h"
#include "measurements.h"
#include "status.h"
#include "gps.h"
#include "packets.h"

/* Timestamps */
static uint32_t time_capture_pps_timestamp;
static uint32_t time_capture_radio_timestamp;

/* Semaphores */
binary_semaphore_t pps_event_sem;
binary_semaphore_t radio_sync_event_sem;

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
    
    uint32_t time_of_flight = 0;
    
    while(TRUE) {
    
        
    
        /* Wait for PPS Event */
        if (chBSemWaitTimeout(&pps_event_sem, MS2ST(1500)) == MSG_TIMEOUT) {
        
            continue;    
        }        
        
        
        /* Wait for NAV-PVT Message */
        if (chBSemWaitTimeout(&pvt_ready_sem, MS2ST(1500)) == MSG_TIMEOUT) {
        
            continue;
        }
        
        /* Timestamp Packet */
        range_pkt.time_of_week = pvt_latest.i_tow;
               
        
        /* Wait for Radio Sync Event */
        if (chBSemWaitTimeout(&radio_sync_event_sem, MS2ST(1500)) == MSG_TIMEOUT) {
        
            continue;
        }
        
        
        /* Compute Time of Flight */
        time_of_flight = (time_capture_pps_timestamp - time_capture_radio_timestamp);        
        range_pkt.type = (PACKET_RANGE | TOAD_ID);
        range_pkt.tof = time_of_flight;
    
        /* TODO - Log data somewhere */
        (void)range_pkt;
    }
}


/* Init Measurements */
void measurement_init(void) {
    
    /* Create Semaphores */
    chBSemObjectInit(&pps_event_sem, TRUE);
    chBSemObjectInit(&pvt_ready_sem, TRUE);
    chBSemObjectInit(&radio_sync_event_sem, TRUE);
    
    /* Create Thread */
    chThdCreateStatic(waMEASUREThread, sizeof(waMEASUREThread), NORMALPRIO, MEASUREThread, NULL);
}
