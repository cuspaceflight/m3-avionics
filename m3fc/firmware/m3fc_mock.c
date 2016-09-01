#include "m3fc_mock.h"
#include "m3fc_status.h"

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

void m3fc_mock_handle_enable(uint8_t* data, uint8_t datalen) {
    (void)data;
    (void)datalen;

    m3status_set_error(M3FC_COMPONENT_MOCK, M3FC_ERROR_MOCK_ENABLED);
    m3fc_mock_set_enabled();
}

void m3fc_mock_handle_accel(uint8_t* data, uint8_t datalen) {
    if(datalen != 6) {
        m3status_set_error(M3FC_COMPONENT_MOCK, M3FC_ERROR_CAN_BAD_COMMAND);
        return;
    }

    uint16_t accels[3];
    accels[0] = data[0] | data[1]<<8;
    accels[1] = data[2] | data[3]<<8;
    accels[2] = data[4] | data[5]<<8;
    m3fc_mock_set_accel(accels[0], accels[1], accels[2]);
}

void m3fc_mock_handle_baro(uint8_t* data, uint8_t datalen) {
    if(datalen != 8) {
        m3status_set_error(M3FC_COMPONENT_MOCK, M3FC_ERROR_CAN_BAD_COMMAND);
        return;
    }

    int32_t pressure, temperature;
    pressure = data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24;
    temperature = data[4] | data[5] << 8 | data[6] << 16 | data[7] << 24;
    m3fc_mock_set_baro(pressure, temperature);
}
