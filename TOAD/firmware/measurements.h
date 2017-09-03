#ifndef MEASURE_H
#define MEASURE_H

#include "packets.h"
#include "gps.h"

/* Measurement Functions */
void measurements_handle_pps(void);
void measurements_handle_radio(void);

/* Timestamped SYNC Events */
typedef struct __attribute__((packed)) {    
    pvt_capture pps_event;
    uint32_t sync_event[50];
} sync_capture;

/* Measurements Init */
void measurement_init(void);

/* Global Ranging Packet */
extern ranging_packet range_pkt;

/* Ranging Packet Mutex */
extern mutex_t range_pkt_mutex;

/* PPS Timestamp */
extern uint32_t time_capture_pps_timestamp;

/* PPS Event Semaphore */
extern binary_semaphore_t pps_event_sem;

/* Telemetry Activity Flag */
extern bool telem_activity;

#endif
