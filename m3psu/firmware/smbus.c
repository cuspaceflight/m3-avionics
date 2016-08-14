/*
 * Generic SMBus Driver
 * Cambridge University Spaceflight
 */

#include <string.h>

#include "error.h"
#include "smbus.h"

uint8_t smbus_write_byte(I2CDriver *i2c, uint8_t deviceaddress, uint8_t byteaddress, uint8_t value){
  uint8_t txdat[2];

  txdat[0] = byteaddress;
  txdat[1] = value;

  i2cAcquireBus(i2c);
  msg_t status = i2cMasterTransmitTimeout(i2c, deviceaddress, txdat, sizeof(txdat), NULL, 0, MS2ST(2));
  i2cReleaseBus(i2c);

  if(status == MSG_OK){
    return ERR_OK;
  }
  return ERR_COMMS;
}

uint8_t smbus_read_byte(I2CDriver *i2c, uint8_t deviceaddress, uint8_t byteaddress, uint8_t *result){
  uint8_t txdat[1];
  uint8_t rxdat[1];

  txdat[0] = byteaddress;

  i2cAcquireBus(i2c);
  msg_t status = i2cMasterTransmitTimeout(i2c, deviceaddress, txdat, sizeof(txdat), rxdat, sizeof(rxdat), MS2ST(2));
  i2cReleaseBus(i2c);

  if(status == MSG_OK){
    *result = rxdat[0];
    return ERR_OK;
  }
  return ERR_COMMS;
}

uint8_t smbus_write_word(I2CDriver *i2c, uint8_t deviceaddress, uint8_t byteaddress, uint16_t value){
  uint8_t txdat[3];
  txdat[0] = byteaddress;
  txdat[1] = (value >> 8) & 0xff;
  txdat[2] = value & 0xff;

  i2cAcquireBus(i2c);
  msg_t status = i2cMasterTransmitTimeout(i2c, deviceaddress, txdat, sizeof(txdat), NULL, 0, MS2ST(2));
  i2cReleaseBus(i2c);

  if(status == MSG_OK){
    return ERR_OK;
  }
  return ERR_COMMS;
}

uint8_t smbus_read_word(I2CDriver *i2c, uint8_t deviceaddress, uint8_t byteaddress, uint16_t *result){
  uint8_t txdat[1];
  uint8_t rxdat[2];

  txdat[0] = byteaddress;

  i2cAcquireBus(i2c);
  msg_t status = i2cMasterTransmitTimeout(i2c, deviceaddress, txdat, sizeof(txdat), rxdat, sizeof(rxdat), MS2ST(2));
  i2cReleaseBus(i2c);

  if(status == MSG_OK){
    *result = ((rxdat[0] << 8) | rxdat[1]);
    return ERR_OK;
  }
  return ERR_COMMS;
}

uint8_t smbus_write_block(I2CDriver *i2c, uint8_t deviceaddress, uint8_t byteaddress, uint8_t *data, uint8_t datalen){
  uint8_t txdat[datalen + 2];

  txdat[0] = byteaddress;
  txdat[1] = datalen + 2;

  // copy rest of data
  memcpy(&txdat[2], data, datalen);

  i2cAcquireBus(i2c);
  msg_t status = i2cMasterTransmitTimeout(i2c, deviceaddress, txdat, datalen+2, NULL, 0, MS2ST(2));
  i2cReleaseBus(i2c);

  if (status == MSG_OK) {
    return ERR_OK;
  }
  return ERR_COMMS;
}

uint8_t smbus_read_block(I2CDriver *i2c, uint8_t deviceaddress, uint8_t byteaddress, uint8_t *txdat, uint8_t txdatlen, uint8_t *data, uint8_t datalen){
  uint8_t cmd[txdatlen + 2];

  cmd[0] = byteaddress;
  cmd[1] = txdatlen + 2;

  // copy rest of tx-data
  memcpy(&cmd[2], txdat, txdatlen);

  i2cAcquireBus(i2c);
  msg_t status = i2cMasterTransmitTimeout(i2c, deviceaddress, cmd, txdatlen+2, data, datalen, MS2ST(2));
  i2cReleaseBus(i2c);

  if (status == MSG_OK) {
    return ERR_OK;
  }
  return ERR_COMMS;
}
