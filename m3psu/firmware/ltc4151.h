/*
 * LTC4151 Driver
 * Cambridge University Spaceflight
 */

#ifndef LTC4151_H_
#define LTC4151_H_

#include "ch.h"
#include "hal.h"

typedef struct {
  I2CDriver *i2c;
  i2caddr_t address;
  float rsense_ohms;
} LTC4151Config;

typedef struct {
  LTC4151Config config;
  float voltage_v;
  float current_ma;
  float power_mw;
  systime_t poll_time;
} LTC4151;

uint8_t ltc4151_init(LTC4151 *ltc, I2CDriver *i2c, i2caddr_t address, float sense_resistor);
uint8_t ltc4151_poll(LTC4151 *ltc);

#endif /* LTC4151_H_ */
