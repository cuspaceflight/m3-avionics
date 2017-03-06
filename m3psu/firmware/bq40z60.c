/*
 * BQ40Z60 Driver
 * Cambridge University Spaceflight
 */

#include <string.h>

#include "error.h"
#include "smbus.h"
#include "bq40z60.h"

//////// PROTECTIONS CONFIG ////////

//// Settings:Protection:Enabled Protections B
// OTC -> 0 (disable charge overtemperature protection)
// OTD -> 0 (disable discharge overtemperature protection)

//// Settings:Protection:Enabled Protections C
// OTF -> 0 (Disable FET overtemperature protection)

//// Settings:Protection:Enabled Protections D
// UTC -> 0 (disable charge undertemperature protection)
// UTD -> 0 (disable discharge undertemperature protection)

//// Settings:Configuration:Temperature Enable
// TS{0-4} Enable -> 0 (Disabled)
// TSInt Enable -> 1 (Enabled)

//// Settings:Configuration:Temperature Mode
// TSInt Mode -> 0 (CELL)


/////// CHARGER SETTINGS ////////

// Settings:Configuration:Mfg Status Init
// CHGR_EN -> 0 (default)

// Advanced Charge Algorithm:Charger:Minimum Voltage Output -> 4192 (610 * (1 + (330k / 56.2k)))
// Advanced Charge Algorithm:Charger:Voltage Resolution -> 16 ((610 * (1 + (330k / 56.2k))) / 256)
// Advanced Charge Algorithm:Charger:Current Resolution -> 39 (0.39 / 0.01ohm)
// Advanced Charge Algorithm:Charger:Max Current Register -> 255 (default) (ILimit * 0.01ohm * 2550)

//// Settings:Configuration:DA Configuration
// CC (cell count) -> 1 (value is num cells - 1)

// Advanced Charge Algorithm:Standard Temp Charging:Current Low -> 990 mA
// Advanced Charge Algorithm:Standard Temp Charging:Current Med -> 1000 mA
// Advanced Charge Algorithm:Standard Temp Charging:Current High -> 1000 mA

// Advanced Charge Algorithm:Rec Temp Charging:Current Low -> 1000 mA
// Advanced Charge Algorithm:Rec Temp Charging:Current Med -> 1000 mA
// Advanced Charge Algorithm:Rec Temp Charging:Current High -> 1000 mA

// Advanced Charge Algorithm:High Temp Charging:Current Low -> 506 mA
// Advanced Charge Algorithm:High Temp Charging:Current Med -> 990 mA
// Advanced Charge Algorithm:High Temp Charging:Current High -> 748 mA


//////// IO CONFIG ////////

//// Settings:Manufacturing:Mfg Status Init
// LED_EN -> 0 (default)
// FET_EN -> 1 (allow enabling of FETs)

//// Settings:Configuration:DA Configuration
// NR -> 1 (non-removable)


//////// GAUGING CONFIG ////////

//// Settings:Manufacturing:Mfg Status Init
// GAUGE_EN -> 1

// Gas Gauging:State:Qmax Cell 1 -> 2200 mAh
// Gas Gauging:State:Qmax Cell 2 -> 2200 mAh
// Gas Gauging:State:Qmax Cell 3 -> 2200 mAh
// Gas Gauging:State:Qmax Cell 4 -> 2200 mAh
// Gas Gauging:State:Qmax Pack -> 2200 mAh

// Gas Gauging:Design:Design Capacity mAh -> 2200mAh
// Gas Gauging:Design:Design Capacity cWh -> 1628 cWh
// Gas Gauging:Design:Design Voltage -> 7400 mV

// Gas Gauging:IT Cfg:Load Mode -> 1 (Constant Power)
// Gas Gauging:IT Cfg:Load Select -> 7 (default) (Max Avg P Last Run) (apparently best for variable loads)
// Gas Gauging:IT Cfg:Term Voltage -> 6000 mV (2 cells * 3000mV/cell)

//// Settings:Configuration:IO Config
// BTP_POL -> 0 (default) (assert low)
// BTP_EN -> 1 (enable BTP interrupt)

//(*** TEST THIS -> MAYBE LEAVE AS 00? ***)
//// Gas Gauging:State:Update Status
// Enable -> 1 (IT and gauging enabled)
// UPDATE -> 1 (QMax updates allowed, but not Ra table updates)


//////// BALANCING CONFIG ////////

//// Settings:Configuration:Balancing Configuration
// CB -> 1 (default) (Enable balancing)
// CBR -> 0 (default) (Disable balancing at rest)


/////////// REGISTERS TO USE
// MAC DAStatus1 gives cell voltages, power etc.
// MAC DAStatus2 gives temperatures
// RelativeStateOfCharge (RSOC)
// FullChargeCapacity (FCC)
// RemainingCapacity (RemCap)
// BatteryStatus gives DSG (1 if not charging), other flags (e.g. protections)
// OperationStatus give XCHG (1 if charging prohibited - see manual 4.12)
// ITStatus1, ITStatus2, ITStatus3 for gauging info (for debug only?)

#define BQ40Z60_CMD_TEMPERATURE               0x08
#define BQ40Z60_CMD_CURRENT                   0x0A
#define BQ40Z60_CMD_RELATIVE_STATE_OF_CHARGE  0x0D
#define BQ40Z60_CMD_RUN_TIME_TO_EMPTY         0x11
#define BQ40Z60_CMD_MANUFACTURER_ACCESS       0x44
#define BQ40Z60_CMD_OPERATION_STATUS          0x54
#define BQ40Z60_CMD_DASTATUS1                 0x71

#define BQ40Z60_MAC_OPERATION_STATUS        0x0054
#define BQ40Z60_MAC_CHARGING_STATUS         0x0055
#define BQ40Z60_MAC_GAUGING_STATUS          0x0056
#define BQ40Z60_MAC_MANUFACTURING_STATUS    0x0057
#define BQ40Z60_MAC_CHGR_EN_TOGGLE          0x00C0

#define BQ40Z60_MAC_MANUFACTURING_STATUS_HI_CHGR_EN_MASK    (1 << 2)
#define BQ40Z60_GAUGING_STATUS_B0_DSG_MASK                  (1 << 6)

#define BQ40Z60_DF_ENABLED_PROTECTIONS_A    0x4845
#define BQ40Z60_DF_AOLD_THRESHOLD           0x4960

uint8_t bq40z60_mac_write(BQ40Z60 *bq, uint16_t mac_address, uint8_t *txbuf, uint8_t txbuflen){
  uint8_t txdat[64];

  chDbgAssert(txbuflen <= 62, "txbuflen > 62");

  txdat[0] = mac_address & 0xff;
  txdat[1] = (mac_address >> 8) & 0xff;

  // copy rest of the data to transmit
  memcpy(txdat+2, txbuf, txbuflen);

  return smbus_write_block(bq->config.i2c, bq->config.address, BQ40Z60_CMD_MANUFACTURER_ACCESS, txdat, txbuflen+2);
}

uint8_t bq40z60_mac_read(BQ40Z60 *bq, uint16_t mac_address, uint8_t *rxbuf, uint8_t rxbuflen){
  uint8_t txdat[2];
  uint8_t rxdat[64];

  // Tell the chip which address we want to read
  txdat[0] = mac_address & 0xff;
  txdat[1] = (mac_address >> 8) & 0xff;
  msg_t status = smbus_write_block(bq->config.i2c, bq->config.address, 0x44, txdat, sizeof(txdat));
  if(status != 0){
      return status;
  }

  // Read the data back from the chip
  status = smbus_read_block(bq->config.i2c, bq->config.address, 0x44, NULL, 0, rxdat, rxbuflen + 2);
  if(status != 0){
      return status;
  }

  // Copy the data to the output buffer
  memcpy(rxbuf, rxdat+2, rxbuflen);

  return 0;
}

uint8_t bq40z60_set_charger_enabled(BQ40Z60 *bq, uint8_t enabled){
  // Check if charger is already enabled and toggle it if necessary
  uint8_t is_enabled = 0;
  if(bq40z60_is_charger_enabled(bq, &is_enabled) != ERR_OK){
      return ERR_COMMS;
  }
  if(is_enabled != enabled){
      return bq40z60_mac_write(bq, BQ40Z60_MAC_CHGR_EN_TOGGLE, NULL, 0);
  }
  return ERR_OK;
}

uint8_t bq40z60_is_charger_enabled(BQ40Z60 *bq, uint8_t *enabled){
  uint8_t rxdat[2];
  if(bq40z60_mac_read(bq, BQ40Z60_MAC_MANUFACTURING_STATUS, rxdat, sizeof(rxdat))!=ERR_OK){
      return ERR_COMMS;
  }

  if((rxdat[1] & BQ40Z60_MAC_MANUFACTURING_STATUS_HI_CHGR_EN_MASK) != 0){
      *enabled = 1;
  }else{
      *enabled = 0;
  }

  return ERR_OK;
}

uint8_t bq40z60_get_cell_voltages(BQ40Z60 *bq, float *cell1, float *cell2, float *batt){
  uint8_t rxbuf[32];
  if(smbus_read_block(bq->config.i2c, bq->config.address, BQ40Z60_CMD_DASTATUS1, NULL, 0, rxbuf, sizeof(rxbuf)) != ERR_OK){
    return ERR_COMMS;
  }

  *cell1 = (float) (rxbuf[0] | (rxbuf[1] << 8)) / 1000.0f;
  *cell2 = (float) (rxbuf[2] | (rxbuf[3] << 8)) / 1000.0f;
  *batt  = (float) (rxbuf[8] | (rxbuf[9] << 8)) / 1000.0f;

  return ERR_OK;
}

uint8_t bq40z60_get_current(BQ40Z60 *bq, int16_t *ma){
  if(smbus_read_word(bq->config.i2c, bq->config.address, BQ40Z60_CMD_CURRENT, (uint16_t *)ma) != ERR_OK){
    return ERR_COMMS;
  }

  return ERR_OK;
}

uint8_t bq40z60_get_run_time_to_empty(BQ40Z60 *bq, uint16_t *mins){
  if(smbus_read_word(bq->config.i2c, bq->config.address, BQ40Z60_CMD_RUN_TIME_TO_EMPTY, mins) != ERR_OK){
    return ERR_COMMS;
  }

  return ERR_OK;
}

uint8_t bq40z60_get_rsoc(BQ40Z60 *bq, uint8_t *percent){
  uint16_t perc = 0;
  if(smbus_read_word(bq->config.i2c, bq->config.address, BQ40Z60_CMD_RELATIVE_STATE_OF_CHARGE, &perc) != ERR_OK){
    return ERR_COMMS;
  }

  *percent = perc & 0xff;

  return ERR_OK;
}

uint8_t bq40z60_get_temperature(BQ40Z60 *bq, uint16_t *cK){
  if(smbus_read_word(bq->config.i2c, bq->config.address, BQ40Z60_CMD_TEMPERATURE, cK) != ERR_OK){
    return ERR_COMMS;
  }
  return ERR_OK;
}

uint8_t bq40z60_get_charging_status(BQ40Z60 *bq, uint8_t *chgstatus, uint8_t chgstatuslen){
  // The bq40z60 wants us to read 4 bytes, but the data is only 3 bytes
  // It gives back a dummy first byte then 3 valid bytes
  uint8_t rxdat[4];
  chDbgAssert(chgstatuslen == 3, "chgstatuslen != 3");
  if(bq40z60_mac_read(bq, BQ40Z60_MAC_CHARGING_STATUS, rxdat, sizeof(rxdat)) != ERR_OK){
    return ERR_COMMS;
  }
  memcpy(chgstatus, rxdat+1, 3); // Discard the first byte
  return ERR_OK;
}

uint8_t bq40z60_get_operation_status(BQ40Z60 *bq, uint8_t *opstatus, uint8_t opstatuslen){
  chDbgAssert(opstatuslen == 4, "opstatuslen != 4");
  if(bq40z60_mac_read(bq, BQ40Z60_MAC_OPERATION_STATUS, opstatus, opstatuslen) != ERR_OK){
    return ERR_COMMS;
  }
  return ERR_OK;
}

uint8_t bq40z60_is_discharging(BQ40Z60 *bq, bool *status){
  uint8_t rxbuf[3];
  if(bq40z60_mac_read(bq, BQ40Z60_MAC_GAUGING_STATUS, rxbuf, sizeof(rxbuf)) != ERR_OK){
    return ERR_COMMS;
  }

  *status = (rxbuf[0] & BQ40Z60_GAUGING_STATUS_B0_DSG_MASK) != 0;

  return ERR_OK;
}

uint8_t bq40z60_write_dataflash(BQ40Z60 *bq, uint16_t addr, uint8_t *data, uint8_t datalen){
  chDbgAssert(datalen <= 32, "datalen > 32");
  chDbgAssert(addr >= 0x4000 && addr <= 0x5FFF, "DF address OOB");

  msg_t status = bq40z60_mac_write(bq, addr, data, datalen);
  if(status == ERR_OK){
    chThdSleepMilliseconds(500); // Wait for flash to save
  }

  return status;
}

uint8_t bq40z60_read_dataflash(BQ40Z60 *bq, uint16_t addr, uint8_t *data, uint8_t datalen){
  chDbgAssert(datalen <= 32, "datalen > 32");
  chDbgAssert(addr >= 0x4000 && addr <= 0x5FFF, "DF address OOB");

  uint8_t rxdat[32];
  msg_t status = bq40z60_mac_read(bq, addr, rxdat, datalen);
  if(status != ERR_OK){
    return status;
  }

  memcpy(data, rxdat, datalen);

  return ERR_OK;
}

uint8_t bq40z60_verify_dataflash(BQ40Z60 *bq, uint16_t addr, uint8_t *data, uint8_t datalen){
  chDbgAssert(datalen <= 32, "datalen > 32");
  chDbgAssert(addr >= 0x4000 && addr <= 0x5FFF, "DF address OOB");

  uint8_t rx[32];
  msg_t status = bq40z60_read_dataflash(bq, addr, rx, sizeof(rx));
  if(status != ERR_OK){
    return status;
  }

  for(int i=0; i<datalen; i++){
    if(rx[i] != data[i]){
      return ERR_INVAL;
    }
  }

  return ERR_OK;
}

uint8_t bq40z60_write_and_verify_dataflash(BQ40Z60 *bq, uint16_t addr, uint8_t *data, uint8_t datalen){
  chDbgAssert(datalen <= 32, "datalen > 32");
  chDbgAssert(addr >= 0x4000 && addr <= 0x5FFF, "DF address OOB");

  msg_t status = bq40z60_write_dataflash(bq, addr, data, datalen);
  if(status != ERR_OK){
    return status;
  }

  return bq40z60_verify_dataflash(bq, addr, data, datalen);
}

uint8_t bq40z60_disable_all_protections(BQ40Z60 *bq){
  uint8_t zeroes[4] = {0,0,0,0}; // All protections disabled
  msg_t status = bq40z60_write_and_verify_dataflash(bq, BQ40Z60_DF_ENABLED_PROTECTIONS_A, zeroes, sizeof(zeroes));
  if(status != ERR_OK){
    return status;
  }

  uint8_t aold_threshold[1] = {0xfb}; // 8A current limit for 31ms
  status = bq40z60_write_and_verify_dataflash(bq, BQ40Z60_DF_AOLD_THRESHOLD, aold_threshold, sizeof(aold_threshold));
  if(status != ERR_OK){
    return status;
  }

  return ERR_OK;
}

uint8_t bq40z60_enable_default_protections(BQ40Z60 *bq){
  uint8_t defaults[4] = {0xff, 0x7f, 0xd5, 0x2f}; // All protections enabled
  msg_t status = bq40z60_write_and_verify_dataflash(bq, BQ40Z60_DF_ENABLED_PROTECTIONS_A, defaults, sizeof(defaults));
  if(status != ERR_OK){
    return status;
  }

  uint8_t aold_threshold[1] = {0xf4}; // 4A current limit for 31ms
  status = bq40z60_write_and_verify_dataflash(bq, BQ40Z60_DF_AOLD_THRESHOLD, aold_threshold, sizeof(aold_threshold));
  if(status != ERR_OK){
    return status;
  }

  return ERR_OK;
}

uint8_t bq40z60_are_default_protections_enabled(BQ40Z60 *bq, bool *enabled){
  uint8_t defaults[4] = {0xff, 0x7f, 0xd5, 0x2f};

  msg_t status = bq40z60_verify_dataflash(bq, BQ40Z60_DF_ENABLED_PROTECTIONS_A, defaults, sizeof(defaults));
  if(status == ERR_INVAL){
    *enabled = false;
    return ERR_OK;
  }else if(status != ERR_OK){
    return status;
  }

  uint8_t aold_threshold[1] = {0xf4};
  status = bq40z60_verify_dataflash(bq, BQ40Z60_DF_AOLD_THRESHOLD, aold_threshold, sizeof(aold_threshold));
  if(status == ERR_INVAL){
    *enabled = false;
    return ERR_OK;
  }else if(status != ERR_OK){
    return status;
  }

  *enabled = true;
  return ERR_OK;
}

uint8_t bq40z60_device_reset(BQ40Z60 *bq){
  return bq40z60_mac_write(bq, 0x0041, NULL, 0);
}

uint8_t bq40z60_init(BQ40Z60 *bq, I2CDriver *i2c, i2caddr_t address){
  bq->config.i2c = i2c;
  bq->config.address = address;

  return ERR_OK;
}

