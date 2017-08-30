#ifndef LOGGING_H
#define LOGGING_H

#include "psu.h"
#include "packets.h"
#include "gps.h"

/* Log Message Types */
#define MESSAGE_PVT         0x01
#define MESSAGE_PSU         0x02
#define MESSAGE_RANGING     0x04
#define MESSAGE_POSITION    0x08
#define MESSAGE_TELEM_1     0x10
#define MESSAGE_TELEM_2     0x20

/* TOAD Log Message */
typedef struct __attribute__((packed)) {

    uint8_t type;
    uint8_t id;
    uint8_t payload[126]; 
    
} toad_log;


/* Logging Functions */
void log_pvt(ublox_pvt_t *pvt_data);
void log_psu_status(psu_status *bat_data);
void log_ranging_packet(ranging_packet *range_data);
void log_position_packet(position_packet *position_data);
void log_telem_packet(uint8_t* buff); 

/* Start Logging Thread */
void logging_init(void);

#endif
