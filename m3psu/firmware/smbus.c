/*
 * Generic SMBus Driver
 * Cambridge University Spaceflight
 */

#include <string.h>

#include "error.h"
#include "smbus.h"

uint8_t smbus_write_byte(I2CDriver *i2c, uint8_t deviceaddress, uint8_t byteaddress, uint8_t value){
  static uint8_t txdat[2] __attribute__((section("DATA_RAM"))); // Can't DMA from Core-Coupled Memory (where the stack resides)

  txdat[0] = byteaddress;
  txdat[1] = value;

  i2cAcquireBus(i2c);
  msg_t status = i2cMasterTransmitTimeout(i2c, deviceaddress, txdat, 2, NULL, 0, MS2ST(20));
  i2cReleaseBus(i2c);

  if(status == MSG_OK){
    return ERR_OK;
  }
  return ERR_COMMS;
}

uint8_t smbus_read_byte(I2CDriver *i2c, uint8_t deviceaddress, uint8_t byteaddress, uint8_t *result){
  static uint8_t txdat[1] __attribute__((section("DATA_RAM")));
  static uint8_t rxdat[1] __attribute__((section("DATA_RAM")));

  txdat[0] = byteaddress;

  i2cAcquireBus(i2c);
  msg_t status = i2cMasterTransmitTimeout(i2c, deviceaddress, txdat, 1, rxdat, 1, MS2ST(20));
  i2cReleaseBus(i2c);

  if(status == MSG_OK){
    *result = rxdat[0];
    return ERR_OK;
  }
  return ERR_COMMS;
}

uint8_t smbus_write_word(I2CDriver *i2c, uint8_t deviceaddress, uint8_t byteaddress, uint16_t value){
  static uint8_t txdat[3] __attribute__((section("DATA_RAM")));
  txdat[0] = byteaddress;
  txdat[1] = value & 0xff;
  txdat[2] = (value >> 8) & 0xff;

  i2cAcquireBus(i2c);
  msg_t status = i2cMasterTransmitTimeout(i2c, deviceaddress, txdat, 3, NULL, 0, MS2ST(20));
  i2cReleaseBus(i2c);

  if(status == MSG_OK){
    return ERR_OK;
  }
  return ERR_COMMS;
}

uint8_t smbus_read_word(I2CDriver *i2c, uint8_t deviceaddress, uint8_t byteaddress, uint16_t *result){
  static uint8_t rxdat[2] __attribute__((section("DATA_RAM")));
  static uint8_t txdat[1] __attribute__((section("DATA_RAM")));

  txdat[0] = byteaddress;

  i2cAcquireBus(i2c);
  msg_t status = i2cMasterTransmitTimeout(i2c, deviceaddress, txdat, 1, rxdat, 2, MS2ST(20));
  i2cReleaseBus(i2c);

  if(status == MSG_OK){
    *result = ((rxdat[1] << 8) | rxdat[0]);
    return ERR_OK;
  }
  return ERR_COMMS;
}

uint8_t smbus_write_block(I2CDriver *i2c, uint8_t deviceaddress, uint8_t byteaddress, uint8_t *data, uint8_t datalen){
  static uint8_t txdat[64] __attribute__((section("DATA_RAM")));

  // TODO: find actual debug-assert command, use that instead
  if(datalen > 62){
    return ERR_COMMS;
  }

  txdat[0] = byteaddress;
  txdat[1] = datalen;

  // copy rest of data
  memcpy(txdat+2, data, datalen);

  i2cAcquireBus(i2c);
  msg_t status = i2cMasterTransmitTimeout(i2c, deviceaddress, txdat, datalen+2, NULL, 0, MS2ST(20));
  i2cReleaseBus(i2c);

  if (status == MSG_OK) {
    return ERR_OK;
  }
  return ERR_COMMS;
}

uint8_t smbus_read_block(I2CDriver *i2c, uint8_t deviceaddress, uint8_t byteaddress, uint8_t *txdat, uint8_t txdatlen, uint8_t *data, uint8_t datalen){
  static uint8_t cmd[64] __attribute__((section("DATA_RAM")));
  static uint8_t recv[64] __attribute__((section("DATA_RAM")));

  // TODO: find actual debug-assert command, use that instead
  if(txdatlen > 62 || datalen > 63){
    return ERR_COMMS;
  }

  cmd[0] = byteaddress;
  cmd[1] = txdatlen;

  // copy rest of tx-data
  memcpy(cmd+2, txdat, txdatlen);

  i2cAcquireBus(i2c);
  msg_t status = i2cMasterTransmitTimeout(i2c, deviceaddress, cmd, txdatlen+2, recv, datalen+1, MS2ST(20));
  i2cReleaseBus(i2c);

  if (status == MSG_OK) {
    // First byte read is the length of data being returned by the device.
    uint8_t numbytes = (datalen < recv[0]) ? datalen : recv[0];
    memcpy(data, recv+1, numbytes);
    return ERR_OK;
  }
  return ERR_COMMS;
}
