#include "ch.h"
#include "hal.h"

#include "timer.h"
#include "measurements.h"

/* Timestamps */
static uint32_t time_capture_pps_timestamp;
static uint32_t time_capture_radio_timestamp;

/* Measurements Ready Semaphore */
binary_semaphore_t tof_ready_sem;


void measurements_handle_pps(void) {

    /* PPS Timestamp */
    time_capture_pps_timestamp = TIM2->CCR1;
    
    /* TODO - Trigger Backhaul delay */
}


void measurements_handle_radio(void) {

    /* Radio Timestamp */
    time_capture_radio_timestamp = TIM2->CCR2;
    
    /* Signal Semaphore */
	chSysLockFromISR();
	chBSemSignalI(&tof_ready_sem);
	chSysUnlockFromISR();
}


/* Measurements Thread */
static THD_WORKING_AREA(waMEASUREThread, 256);
static THD_FUNCTION(MEASUREThread, arg) {

    (void)arg;
    chRegSetThreadName("Measurements");
    
    static uint32_t time_of_flight = 0;
    
    while(TRUE) {
    
        /* Wait for Time of Flight Measurements */
        chBSemWait(&tof_ready_sem);
        
        /* Compute Time of Flight */
        time_of_flight = (time_capture_pps_timestamp - time_capture_radio_timestamp);
    
        /* TODO - Log TOF data somewhere */
        (void)time_of_flight;
    }
}


/* Init Measurements */
void measurement_init(void) {
    
    /* Create Semaphore */
    chBSemObjectInit(&tof_ready_sem, FALSE);
    
    /* Create Thread */
    chThdCreateStatic(waMEASUREThread, sizeof(waMEASUREThread), NORMALPRIO, MEASUREThread, NULL);
}
