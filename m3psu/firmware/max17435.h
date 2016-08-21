/*
 * MAX17435 Driver
 * Cambridge University Spaceflight
 */

#ifndef MAX17435_H_
#define MAX17435_H_

#include "ch.h"
#include "hal.h"

typedef struct {
  I2CDriver *i2c;
  i2caddr_t address;
  uint16_t charge_voltage_mv;
  uint16_t charge_current_ma;
  uint16_t css_resistor_mohms;
} MAX17435Config;

typedef struct {
  MAX17435Config config;
} MAX17435;

uint8_t max17435_init(MAX17435 *max, I2CDriver *i2c, i2caddr_t address, uint16_t charge_voltage_mv, uint16_t charge_current_ma, uint16_t css_resistor_mohms);
uint8_t max17435_set_charge_voltage(MAX17435 *max, uint16_t mv);
uint8_t max17435_set_charge_current(MAX17435 *max, uint16_t ma);
uint8_t max17435_set_input_current_limit(MAX17435 *max, uint16_t ma);
uint8_t max17435_set_relearn_voltage(MAX17435 *max, uint16_t mv);
uint8_t max17435_get_charge_voltage(MAX17435 *max, uint16_t *mv);
uint8_t max17435_get_charge_current(MAX17435 *max, uint16_t *ma);
uint8_t max17435_get_input_current_limit(MAX17435 *max, uint16_t *ma);
uint8_t max17435_get_relearn_voltage(MAX17435 *max, uint16_t *mv);
uint8_t max17435_get_current(MAX17435 *max, uint16_t *ma);

#endif /* MAX17435_H_ */
