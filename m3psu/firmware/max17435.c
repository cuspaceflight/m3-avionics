/*
 * MAX17435 Driver
 * Cambridge University Spaceflight
 */

#include "error.h"
#include "smbus.h"
#include "max17435.h"

#define MAX17435_CMD_CHARGE_CURRENT     0x14
#define MAX17435_CMD_CHARGE_VOLTAGE     0x15
#define MAX17435_CMD_RELEARN_VOLTAGE    0x3D
#define MAX17435_CMD_IINP_VOLTAGE       0x3E
#define MAX17435_CMD_INPUT_CURRENT      0x3F
#define MAX17435_CMD_MANUFACTURER_ID    0xFE
#define MAX17435_CMD_DEVICE_ID          0xFF

uint8_t max17435_set_charge_voltage(MAX17435 *max, uint16_t mv){
  return smbus_write_word(max->config.i2c, max->config.address, MAX17435_CMD_CHARGE_VOLTAGE, mv);
}

uint8_t max17435_set_charge_current(MAX17435 *max, uint16_t ma){
  return smbus_write_word(max->config.i2c, max->config.address, MAX17435_CMD_CHARGE_CURRENT, ma);
}

uint8_t max17435_set_input_current_limit(MAX17435 *max, uint16_t ma){
  return smbus_write_word(max->config.i2c, max->config.address, MAX17435_CMD_INPUT_CURRENT, ma);
}

uint8_t max17435_set_relearn_voltage(MAX17435 *max, uint16_t mv){
  return smbus_write_word(max->config.i2c, max->config.address, MAX17435_CMD_RELEARN_VOLTAGE, mv);
}

uint8_t max17435_get_charge_voltage(MAX17435 *max, uint16_t *mv){
  return smbus_read_word(max->config.i2c, max->config.address, MAX17435_CMD_CHARGE_VOLTAGE, mv);
}

uint8_t max17435_get_charge_current(MAX17435 *max, uint16_t *ma){
  return smbus_read_word(max->config.i2c, max->config.address, MAX17435_CMD_CHARGE_CURRENT, ma);
}

uint8_t max17435_get_input_current_limit(MAX17435 *max, uint16_t *ma){
  return smbus_read_word(max->config.i2c, max->config.address, MAX17435_CMD_INPUT_CURRENT, ma);
}

uint8_t max17435_get_relearn_voltage(MAX17435 *max, uint16_t *mv){
  return smbus_read_word(max->config.i2c, max->config.address, MAX17435_CMD_RELEARN_VOLTAGE, mv);
}

uint8_t max17435_get_current(MAX17435 *max, uint16_t *ma){
  uint16_t mv_read;
  if(smbus_read_word(max->config.i2c, max->config.address, MAX17435_CMD_IINP_VOLTAGE, &mv_read) != 0){
    return ERR_COMMS;
  }

  float mv_sense = (float)mv_read / 20.0f; // The read value is 20x the sense
  float ohms = (float)max->config.css_resistor_mohms / 1000.0f;

  *ma = (uint16_t)(mv_sense / ohms);

  return ERR_OK;
}

uint8_t max17435_init(MAX17435 *max, I2CDriver *i2c, i2caddr_t address, uint16_t charge_voltage_mv, uint16_t charge_current_ma, uint16_t css_resistor_mohms){
  max->config.i2c = i2c;
  max->config.address = address;
  max->config.charge_voltage_mv = charge_voltage_mv;
  max->config.charge_current_ma = charge_current_ma;
  max->config.css_resistor_mohms = css_resistor_mohms;

  return ERR_OK;
}

