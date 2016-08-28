#include "m3fc_mock.h"

static volatile bool m3fc_mock_enable = false;
static volatile int16_t m3fc_mock_accel_x, m3fc_mock_accel_y, m3fc_mock_accel_z;
static volatile int32_t m3fc_mock_pressure, m3fc_mock_temperature;

void m3fc_mock_set_enabled(void) {
    m3fc_mock_enable = true;
}

bool m3fc_mock_get_enabled(void) {
    return m3fc_mock_enable;
}

void m3fc_mock_get_accel(int16_t* accels) {
    accels[0] = m3fc_mock_accel_x;
    accels[1] = m3fc_mock_accel_y;
    accels[2] = m3fc_mock_accel_z;
}

void m3fc_mock_get_baro(int32_t* pressure, int32_t* temperature) {
    *pressure = m3fc_mock_pressure;
    *temperature = m3fc_mock_temperature;
}

void m3fc_mock_set_accel(int16_t accel_x, int16_t accel_y, int16_t accel_z) {
    m3fc_mock_accel_x = accel_x;
    m3fc_mock_accel_y = accel_y;
    m3fc_mock_accel_z = accel_z;
}

void m3fc_mock_set_baro(int32_t pressure, int32_t temperature) {
    m3fc_mock_pressure = pressure;
    m3fc_mock_temperature = temperature;
}
