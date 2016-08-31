#ifndef M3FC_MOCK_H
#define M3FC_MOCK_H
#include <stdint.h>
#include <stdbool.h>

void m3fc_mock_set_enabled(void);
bool m3fc_mock_get_enabled(void);
void m3fc_mock_get_accel(int16_t* accels);
void m3fc_mock_get_baro(int32_t* pressure, int32_t* temperature);
void m3fc_mock_set_accel(int16_t accel_x, int16_t accel_y, int16_t accel_z);
void m3fc_mock_set_baro(int32_t pressure, int32_t temperature);

void m3fc_mock_handle_enable(uint8_t* data, uint8_t datalen);
void m3fc_mock_handle_accel(uint8_t* data, uint8_t datalen);
void m3fc_mock_handle_baro(uint8_t* data, uint8_t datalen);

#endif
