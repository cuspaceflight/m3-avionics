#ifndef MEASURE_H
#define MEASURE_H

#include "ch.h"


/* Measurement Functions */
void measurements_handle_pps(void);
void measurements_handle_radio(void);

/* Measurements Init */
void measurement_init(void);

/* Ranging Packet */
typedef struct __attribute__((packed)) {

    uint8_t ID;
    uint32_t TOF;
    uint32_t i_tow;
    int32_t lon, lat;
    int32_t height,
    
} ranging_packet;

extern ranging_packet range_pkt;

#endif
