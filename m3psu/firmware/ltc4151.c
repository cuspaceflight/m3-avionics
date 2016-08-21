/*
 * LTC4151 Driver
 * Cambridge University Spaceflight
 */

#include "error.h"
#include "smbus.h"
#include "ltc4151.h"

/* Register Addresses */
#define LTC4151_RA_SENSE_HI     0x00
#define LTC4151_RA_SENSE_LO     0x01
#define LTC4151_RA_VIN_HI       0x02
#define LTC4151_RA_VIN_LO       0x03

#define LTC4151_SENSE_SCALE_MV  (0.02f)
#define LTC4151_VIN_SCALE_V     (0.025f)

uint8_t ltc4151_init(LTC4151 *ltc, I2CDriver *i2c, i2caddr_t address, float r_sense_ohms) {
  ltc->config.i2c = i2c;
  ltc->config.address = address;
  ltc->config.rsense_ohms = r_sense_ohms;

  ltc->voltage_v = 0;
  ltc->current_ma = 0;
  ltc->power_mw = 0;
  ltc->poll_time = 0;

  return ERR_OK;
}

uint8_t ltc4151_poll(LTC4151 *ltc) {
  uint16_t dat_le, dat;
  if(smbus_read_word(ltc->config.i2c, ltc->config.address, LTC4151_RA_SENSE_HI, &dat_le) != 0){
    return ERR_COMMS;
  }
  // This reads in the wrong endianness, so swap it:
  dat = ((dat_le & 0xff) << 8) | ((dat_le & 0xff00) >> 8);
  float mv = ((dat >> 4) & 0x0fff) * LTC4151_SENSE_SCALE_MV;
  ltc->current_ma = mv / ltc->config.rsense_ohms;

  if(smbus_read_word(ltc->config.i2c, ltc->config.address, LTC4151_RA_VIN_HI, &dat_le) != 0){
    return ERR_COMMS;
  }
  dat = ((dat_le & 0xff) << 8) | ((dat_le & 0xff00) >> 8);
  ltc->voltage_v = ((dat >> 4) & 0x0fff) * LTC4151_VIN_SCALE_V;

  ltc->power_mw = ltc->voltage_v * ltc->current_ma;

  ltc->poll_time = chVTGetSystemTime();

  return ERR_OK;
}
