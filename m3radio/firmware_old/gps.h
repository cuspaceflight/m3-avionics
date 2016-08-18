/*
 * uBlox GPS receiver
 *
 * M3 Radio
 *
 * gps.h is the header file for the driver for the ublox MAX 7Q gps unit.
 *
 * 2016 Modified by Eivind Roson Eide, Cambridge University Spaceflight
 *
 * Based on:
 * ublox.h
 * M2R
 * 2014 Adam Greig, Cambridge University Spaceflight
 */


 #ifndef UBLOX_H
 #define UBLOX_H

 #include "ch.h"

 typedef struct {
     uint32_t i_tow;
     uint16_t year;
     uint8_t month, day, hour, minute, second;
     uint8_t valid;
     uint32_t t_acc;
     int32_t nano;
     uint8_t fix_type;
     uint8_t flags;
     uint8_t reserved1;
     uint8_t num_sv;
     int32_t lon, lat;
     int32_t height, h_msl;
     uint32_t h_acc, v_acc;
     int32_t velN, velE, velD, gspeed;
     int32_t head_mot;
     uint32_t s_acc;
     uint32_t head_acc;
     uint16_t p_dop;
     uint16_t reserved2;
     uint32_t reserved3;
     int32_t head_veh;
     uint32_t reserved4;
 } __attribute__((packed)) ublox_pvt_t;

 msg_t ublox_thread(void* arg);

 #endif /* UBLOX_H */
