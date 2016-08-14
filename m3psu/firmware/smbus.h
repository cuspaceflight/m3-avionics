/*
 * Generic SMBus Driver
 * Cambridge University Spaceflight
 */

#ifndef SMBUS_H_
#define SMBUS_H_

#include "ch.h"
#include "hal.h"

uint8_t smbus_write_byte(I2CDriver *i2c, uint8_t deviceaddress, uint8_t byteaddress, uint8_t value);
uint8_t smbus_read_byte(I2CDriver *i2c, uint8_t deviceaddress, uint8_t byteaddress, uint8_t *result);
uint8_t smbus_write_word(I2CDriver *i2c, uint8_t deviceaddress, uint8_t byteaddress, uint16_t value);
uint8_t smbus_read_word(I2CDriver *i2c, uint8_t deviceaddress, uint8_t byteaddress, uint16_t *result);
uint8_t smbus_write_block(I2CDriver *i2c, uint8_t deviceaddress, uint8_t byteaddress, uint8_t *data, uint8_t datalen);
uint8_t smbus_read_block(I2CDriver *i2c, uint8_t deviceaddress, uint8_t byteaddress, uint8_t *txdat, uint8_t txdatlen, uint8_t *data, uint8_t datalen);

#endif /* SMBUS_H_ */
