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

#define M3FC_CONFIG_PYRO_USAGE_NONE   (0)
#define M3FC_CONFIG_PYRO_USAGE_DROGUE (1)
#define M3FC_CONFIG_PYRO_USAGE_MAIN   (2)
#define M3FC_CONFIG_PYRO_USAGE_DART   (3)

#define M3FC_CONFIG_PYRO_TYPE_NONE   (0)
#define M3FC_CONFIG_PYRO_TYPE_EMATCH (1)
#define M3FC_CONFIG_PYRO_TYPE_TALON  (2)
#define M3FC_CONFIG_PYRO_TYPE_METRON (3)


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
        /* Pyro channel usage. 0=NONE 1=DROGUE 2=MAIN 3=DART_SEP */
        uint8_t pyro_1_usage, pyro_2_usage, pyro_3_usage, pyro_4_usage;

        /* Pyro channel type, 0=NONE 1=EMATCH 2=TALON 3=METRON */
        uint8_t pyro_1_type, pyro_2_type, pyro_3_type, pyro_4_type;
    } __attribute__((packed)) pyros;
} __attribute__((packed)) __attribute__((aligned(32)));

extern struct m3fc_config m3fc_config;

void m3fc_config_init(void);
void m3fc_config_save(void);
bool m3fc_config_load(void);
bool m3fc_config_check(void);

/* CAN packet handling routines */
void m3fc_config_handle_set_profile(uint8_t* data, uint8_t datalen);
void m3fc_config_handle_set_pyros(uint8_t* data, uint8_t datalen);
void m3fc_config_handle_load(uint8_t* data, uint8_t datalen);
void m3fc_config_handle_save(uint8_t* data, uint8_t datalen);


#endif
