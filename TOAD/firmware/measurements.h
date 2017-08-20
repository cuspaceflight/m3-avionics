#ifndef MEASURE_H
#define MEASURE_H

#include "gps.h"

/* Measurement Functions */
void measurements_handle_pps(void);
void measurements_handle_radio(void);

/* Measurements Init */
void measurement_init(void);

/* Ranging Packet */
typedef struct __attribute__((packed)) {

    uint8_t ID;
    uint32_t TOF;
    ublox_pvt_t position;
    
} ranging_packet;

#endif
