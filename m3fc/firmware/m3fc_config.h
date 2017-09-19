#ifndef M3FC_CONFIG_H
#define M3FC_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#define M3FC_CONFIG_POSITION_DART (1)
#define M3FC_CONFIG_POSITION_CORE (2)

#define M3FC_CONFIG_ACCEL_AXIS_X  (1)
#define M3FC_CONFIG_ACCEL_AXIS_NX (2)
#define M3FC_CONFIG_ACCEL_AXIS_Y  (3)
#define M3FC_CONFIG_ACCEL_AXIS_NY (4)
#define M3FC_CONFIG_ACCEL_AXIS_Z  (5)
#define M3FC_CONFIG_ACCEL_AXIS_NZ (6)

#define M3FC_CONFIG_PYRO_USAGE_MASK         (0xF0)
#define M3FC_CONFIG_PYRO_CURRENT_MASK       (0x0C)
#define M3FC_CONFIG_PYRO_TYPE_MASK          (0x03)
#define M3FC_CONFIG_PYRO_USAGE_NONE         (0x00)
#define M3FC_CONFIG_PYRO_USAGE_DROGUE       (0x10)
#define M3FC_CONFIG_PYRO_USAGE_MAIN         (0x20)
#define M3FC_CONFIG_PYRO_USAGE_DARTSEP      (0x30)
#define M3FC_CONFIG_PYRO_CURRENT_NONE       (0x00)
#define M3FC_CONFIG_PYRO_CURRENT_1A         (0x04)
#define M3FC_CONFIG_PYRO_CURRENT_3A         (0x08)
#define M3FC_CONFIG_PYRO_TYPE_NONE          (0x00)
#define M3FC_CONFIG_PYRO_TYPE_EMATCH        (0x01)
#define M3FC_CONFIG_PYRO_TYPE_TALON         (0x02)
#define M3FC_CONFIG_PYRO_TYPE_METRON        (0x03)

struct m3fc_config {

    struct {
        /* M3FC position. 1=dart 2=core */
        uint8_t m3fc_position;

        /* Accelerometer "up" axis. 1=X 2=-X 3=Y 4=-Y 5=Z 6=-Z */
        uint8_t accel_axis;

        /* Ignition detection acceleration threshold, in m/s/s */
        uint8_t ignition_accel;

        /* Burnout detection timeout, in 0.1s since launch */
        uint8_t burnout_timeout;

        /* Apogee detection timeout, in s since launch */
        uint8_t apogee_timeout;

        /* Altitude to release main parachute, in 10m above launch altitude */
        uint8_t main_altitude;

        /* Main release timeout, in s since apogee */
        uint8_t main_timeout;

        /* Landing detection timeout, in 10s since launch */
        uint8_t land_timeout;
    } __attribute__((packed)) profile;

    struct {
        /* Pyro channels configuration.
         * Top four bits usage: 0000=NONE, 0001=DROGUE, 0010=MAIN, 0011=DARTSEP
         * Next two bits current: 00=NONE, 01=1A, 10=3A
         * Bottom two bits type: 00=NONE, 01=EMATCH, 10=TALON, 11=METRON
         */
        uint8_t pyro1, pyro2, pyro3, pyro4, pyro5, pyro6, pyro7, pyro8;
    } __attribute__((packed)) pyros;

    struct {
        /* Accelerometer scale in g/LSB and 0g offset in LSB for each axis */
        float x_scale, x_offset;
        float y_scale, y_offset;
        float z_scale, z_offset;
    } __attribute__((packed)) accel_cal;

    /* Radio carrier frequency in Hz */
    uint32_t radio_freq;

    /* CRC32 of the whole config */
    uint32_t crc;
} __attribute__((packed)) __attribute__((aligned(4)));

extern struct m3fc_config m3fc_config;

void m3fc_config_init(void);
void m3fc_config_save(void);
bool m3fc_config_load(void);
bool m3fc_config_check(void);

/* CAN packet handling routines */
void m3fc_config_handle_set_profile(uint8_t* data, uint8_t datalen);
void m3fc_config_handle_set_pyros(uint8_t* data, uint8_t datalen);
void m3fc_config_handle_set_accel_cal_x(uint8_t* data, uint8_t datalen);
void m3fc_config_handle_set_accel_cal_y(uint8_t* data, uint8_t datalen);
void m3fc_config_handle_set_accel_cal_z(uint8_t* data, uint8_t datalen);
void m3fc_config_handle_set_radio_freq(uint8_t* data, uint8_t datalen);
void m3fc_config_handle_set_crc(uint8_t* data, uint8_t datalen);
void m3fc_config_handle_load(uint8_t* data, uint8_t datalen);
void m3fc_config_handle_save(uint8_t* data, uint8_t datalen);


#endif
