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

/* Semaphores */
binary_semaphore_t pps_event_sem;
binary_semaphore_t radio_sync_event_sem;

/* Mutexs */
mutex_t range_pkt_mutex;

/* Ranging Packet */
ranging_packet range_pkt;

/* Telemetry Activity Flag */
bool telem_activity;


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

	chSysLockFromISR();
	
	/* Radio Timestamp */
    time_capture_radio_timestamp = TIM2->CCR2;
    
    /* Signal SYNC Semaphore */
	chBSemSignalI(&radio_sync_event_sem);
	
	/* Telemetry Activity */
	telem_activity = TRUE;
	
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
        range_pkt.type = (PACKET_RANGE | TOAD_ID);
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
