#ifndef MEASURE_H
#define MEASURE_H

#include "packets.h"

/* Measurement Functions */
void measurements_handle_pps(void);
void measurements_handle_radio(void);

/* Measurements Init */
void measurement_init(void);

/* Ranging Packet */
extern ranging_packet range_pkt;

#endif
