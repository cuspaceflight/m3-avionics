#ifndef PACKETS_H
#define PACKETS_H

/* Packet Type */
#define PACKET_POSITION     0x80
#define PACKET_RANGE        0x40

/* Ranging Packet */
typedef struct __attribute__((packed)) {

    uint8_t type;
    uint32_t tof;
    uint32_t time_of_week;
    uint8_t bat_volt;
    uint8_t temp;
    
} ranging_packet;


/* Position Packet */
typedef struct __attribute__((packed)) {

    uint8_t type;
    int32_t lon, lat;
    int32_t height;
    uint8_t num_sat;
    uint8_t bat_volt;
    uint8_t temp; 
    
} position_packet;

#endif
