/*
 * LTC2975 Driver
 * Cambridge University Spaceflight
 */

 //TODO: implement current limits?
 
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "config.h"
#include "ltc2975.h"
#include "smbus.h"
#include "pmbus_util.h"

/* Command codes */
#define LTC2975_CMD_PAGE                0x00
#define LTC2975_CMD_OPERATION           0x01
#define LTC2975_CMD_ON_OFF_CONFIG       0x02
#define LTC2975_CMD_CLEAR_FAULTS        0x03
#define LTC2975_CMD_VOUT_MODE           0x20
#define LTC2975_CMD_VOUT_COMMAND        0x21
#define LTC2975_CMD_VOUT_MARGIN_HIGH    0x25
#define LTC2975_CMD_VOUT_MARGIN_LOW     0x26
#define LTC2975_CMD_VIN_ON              0x35 //TODO
#define LTC2975_CMD_VIN_OFF             0x36 //TODO
#define LTC2975_CMD_IOUT_CAL_GAIN       0x38
#define LTC2975_CMD_VOUT_OV_FAULT_LIMIT 0x40
#define LTC2975_CMD_VOUT_OV_WARN_LIMIT  0x42 //TODO
#define LTC2975_CMD_VOUT_UV_WARN_LIMIT  0x43 //TODO
#define LTC2975_CMD_VOUT_UV_FAULT_LIMIT 0x44
#define LTC2975_CMD_UT_FAULT_LIMIT      0x53
#define LTC2975_CMD_UT_FAULT_RESPONSE   0x54
#define LTC2975_CMD_TON_MAX_FAULT_LIMIT 0x62
#define LTC2975_CMD_STATUS_WORD         0x79
#define LTC2975_CMD_STATUS_VOUT         0x7A
#define LTC2975_CMD_STATUS_IOUT         0x7B
#define LTC2975_CMD_STATUS_INPUT        0x7C
#define LTC2975_CMD_STATUS_TEMPERATURE  0x7D
#define LTC2975_CMD_STATUS_CML          0x7E
#define LTC2975_CMD_STATUS_MFR_SPECIFIC 0x80
#define LTC2975_CMD_READ_VIN            0x88
#define LTC2975_CMD_READ_IIN            0x89
#define LTC2975_CMD_READ_VOUT           0x8B
#define LTC2975_CMD_READ_IOUT           0x8C
#define LTC2975_CMD_READ_POUT           0x96
#define LTC2975_CMD_READ_PIN            0x97 //TODO
#define LTC2975_CMD_MFR_PADS            0xE5
#define LTC2975_CMD_MFR_SPECIAL_ID      0xE7
#define LTC2975_CMD_MFR_COMMON          0xEF
#define LTC2975_CMD_MFR_CONFIG_LTC2975  0xD0
#define LTC2975_CMD_MFR_FAULTB0_RESPONSE 0xD5 //TODO
#define LTC2975_CMD_MFR_FAULTB1_RESPONSE 0xD6 //TODO

/* Register descriptions */
#define LTC2975_MFRC_NOT_BUSY_BIT        6 /* Chip not busy */

#define LTC2975_STATUS_VOUT             (1<<15)
#define LTC2975_STATUS_IOUT             (1<<14)
#define LTC2975_STATUS_INPUT            (1<<13)
#define LTC2975_STATUS_MFR_SPECIFIC     (1<<12)
#define LTC2975_STATUS_POWER_NOT_GOOD   (1<<11)
#define LTC2975_STATUS_BUSY             (1<<7)
#define LTC2975_STATUS_OFF              (1<<6)
#define LTC2975_STATUS_VOUT_OV          (1<<5)
#define LTC2975_STATUS_IOUT_OC          (1<<4)
#define LTC2975_STATUS_VIN_UV           (1<<3)
#define LTC2975_STATUS_TEMPERATURE      (1<<2)
#define LTC2975_STATUS_CML              (1<<1)
#define LTC2975_STATUS_OTHER            (1<<0)

#define LTC2975_STATUS_VOUT_VOUT_OV_FAULT       (1<<7)
#define LTC2975_STATUS_VOUT_VOUT_OV_WARNING     (1<<6)
#define LTC2975_STATUS_VOUT_VOUT_UV_WARNING     (1<<5)
#define LTC2975_STATUS_VOUT_VOUT_UV_FAULT       (1<<4)
#define LTC2975_STATUS_VOUT_VOUT_MAX_WARNING    (1<<3)
#define LTC2975_STATUS_VOUT_TON_MAX_FAULT       (1<<2)

#define LTC2975_STATUS_IOUT_IOUT_OC_FAULT       (1<<7)
#define LTC2975_STATUS_IOUT_IOUT_OC_WARNING     (1<<5)
#define LTC2975_STATUS_IOUT_IOUT_UC_FAULT       (1<<4)

#define LTC2975_STATUS_INPUT_VIN_OV_FAULT       (1<<7)
#define LTC2975_STATUS_INPUT_VIN_OV_WARNING     (1<<6)
#define LTC2975_STATUS_INPUT_VIN_UV_WARNING     (1<<5)
#define LTC2975_STATUS_INPUT_VIN_UV_FAULT       (1<<4)
#define LTC2975_STATUS_INPUT_UNIT_OFF_FOR_INSUFFICIENT_VIN (1<<3)

#define LTC2975_STATUS_MFRS_VOUT_DISCHARGE_FAULT (1<<7)
#define LTC2975_STATUS_MFRS_FAULT1_IN           (1<<6)
#define LTC2975_STATUS_MFRS_FAULT0_IN           (1<<5)
#define LTC2975_STATUS_MFRS_WATCHDOG_FAULT      (1<<0)

#define LTC2975_STATUS_TEMP_OT_FAULT            (1<<7)
#define LTC2975_STATUS_TEMP_OT_WARNING          (1<<6)
#define LTC2975_STATUS_TEMP_UT_WARNING          (1<<5)
#define LTC2975_STATUS_TEMP_UT_FAULT            (1<<4)

#define LTC2975_STATUS_CML_INVALID_COMMAND      (1<<7)
#define LTC2975_STATUS_CML_INVALID_DATA         (1<<6)
#define LTC2975_STATUS_CML_PEC_FAILED           (1<<5)
#define LTC2975_STATUS_CML_MEMORY_FAULT         (1<<4)
#define LTC2975_STATUS_CML_PROCESSOR_FAULT      (1<<3)
#define LTC2975_STATUS_CML_OTHER_COMMUNICATIONS_FAULT (1<<1)

#define LTC2975_ONOFF_USE_PMBUS     3 /* Allow on/off control via I2C */

#define LTC2975_OPER_ON             0x80 /* Turn on channel */
#define LTC2975_OPER_OFF            0x40 /* Turn off channel */

#define LTC2975_UT_LIMIT_MIN        0xFDDA /* 0xFDDA = -275C (disable UT warning) */
#define LTC2975_UT_FAULT_IGNORE     0x00 /* Ignore under-temperature faults */ //TODO?

/* Fault bits */
#define LTC2975_FAULT_EXT_UT_BIT    4 //TODO?

/* MFR_CONFIG_LTC2975 bits */
#define LTC2975_WPU_ENABLE  3 //TODO

/* PAGE constants */
#define PAGE_0      0x00
#define PAGE_1      0x01
#define PAGE_2      0x02
#define PAGE_3      0x03
#define PAGE_ALL    0xFF

static int8_t l16_exp;

uint8_t ltc2975_paged_write(LTC2975 *ltc, uint8_t command_code, uint8_t page, uint8_t *data, uint8_t datalen) {
  uint8_t txdat[62];

  chDbgAssert(datalen <= 60, "datalen > 60");

  txdat[0] = page;
  txdat[1] = command_code;

  // copy rest of data
  memcpy(txdat+2, data, datalen);
  return smbus_write_block(ltc->config.i2c, ltc->config.address, LTC2975_CMD_PAGE_PLUS_WRITE, txdat, datalen+2);
}

uint8_t ltc2975_paged_read(LTC2975 *ltc, uint8_t command_code, uint8_t page,
                           uint8_t *data, uint8_t datalen) {
  uint8_t txdat[2];
  txdat[0] = page;
  txdat[1] = command_code;

  return smbus_read_block(ltc->config.i2c, ltc->config.address, LTC2975_CMD_PAGE_PLUS_READ, txdat, sizeof(txdat), data, datalen);
}

uint8_t ltc2975_global_write(LTC2975 *ltc, uint8_t command_code, uint8_t *data,
                             uint8_t datalen) {
  return ltc2975_paged_write(ltc, command_code, PAGE_BOTH, data, datalen);
}

uint8_t ltc2975_global_read(LTC2975 *ltc, uint8_t command_code, uint8_t *data,
                            uint8_t datalen) {
  return ltc2975_paged_read(ltc, command_code, PAGE_BOTH, data, datalen);
}

uint8_t ltc2975_read_l16_exp(LTC2975 *ltc) {
  // Read the VOUT_MODE register to get L16 exponent to use
  uint8_t rxdat[1];
  uint8_t status = ltc2975_global_read(ltc, LTC2975_CMD_VOUT_MODE, rxdat,
                                       sizeof(rxdat));

  // sign-extend 5 bits
  if (rxdat[0] > 0x0F)
    rxdat[0] |= 0xE0;
  l16_exp = rxdat[0];

  return status;
}

bool ltc2975_is_alerting(LTC2975 *ltc) {
  uint8_t mfr_common[1];
  ltc2975_global_read(ltc, LTC2975_CMD_MFR_COMMON, mfr_common,
                      sizeof(mfr_common));

  if ((mfr_common[0] & (1 << LTC2975_MFRC_CNA_BIT)) != 0) {
    return TRUE;
  }
  return FALSE;
}

uint8_t ltc2975_clear_faults(LTC2975 *ltc) {
  // Send a CLEAR_FAULTS command and wait for it to complete
  if (ltc2975_global_write(ltc, LTC2975_CMD_CLEAR_FAULTS, NULL, 0) != ERR_OK) {
    return ERR_COMMS;
  }
  // It can take up to 10us to complete (no way of checking?)
  chThdSleepMicroseconds(10);
  return ERR_OK;
}

ltc2975_fault_status ltc2975_get_fault_status(LTC2975 *ltc, uint8_t page) {
  uint8_t rxdat[2];

  // Read STATUS_WORD to get overview of where faults are
  ltc2975_paged_read(ltc, LTC2975_CMD_STATUS_WORD, page, rxdat, 2);
  uint16_t status_word = (rxdat[1] << 8) | rxdat[0];

  ltc2975_fault_status fault_status;

  if ((status_word & LTC2975_STATUS_VOUT) != 0) {
    fault_status.vout = 1;
    // read STATUS_VOUT
    ltc2975_paged_read(ltc, LTC2975_CMD_STATUS_VOUT, page, rxdat, 1);
    fault_status.vout_ov_fault = ((rxdat[0] & LTC2975_STATUS_VOUT_VOUT_OV_FAULT)
        != 0);
    fault_status.vout_ov_warning = ((rxdat[0]
        & LTC2975_STATUS_VOUT_VOUT_OV_WARNING) != 0);
    fault_status.vout_uv_warning = ((rxdat[0]
        & LTC2975_STATUS_VOUT_VOUT_UV_WARNING) != 0);
    fault_status.vout_uv_fault = ((rxdat[0] & LTC2975_STATUS_VOUT_VOUT_UV_FAULT)
        != 0);
    fault_status.vout_max_warning = ((rxdat[0]
        & LTC2975_STATUS_VOUT_VOUT_MAX_WARNING) != 0);
    fault_status.ton_max_fault = ((rxdat[0] & LTC2975_STATUS_VOUT_TON_MAX_FAULT)
        != 0);
  }
  else {
    fault_status.vout = 0;
    fault_status.vout_ov_fault = 0;
    fault_status.vout_ov_warning = 0;
    fault_status.vout_uv_warning = 0;
    fault_status.vout_uv_fault = 0;
    fault_status.vout_max_warning = 0;
    fault_status.ton_max_fault = 0;
  }

  if ((status_word & LTC2975_STATUS_IOUT) != 0) {
    fault_status.iout = 1;
    // read STATUS_IOUT
    ltc2975_paged_read(ltc, LTC2975_CMD_STATUS_IOUT, page, rxdat, 1);
    fault_status.iout_oc_fault = ((rxdat[0] & LTC2975_STATUS_IOUT_IOUT_OC_FAULT)
        != 0);
    fault_status.iout_oc_warning = ((rxdat[0]
        & LTC2975_STATUS_IOUT_IOUT_OC_WARNING) != 0);
    fault_status.iout_uc_fault = ((rxdat[0] & LTC2975_STATUS_IOUT_IOUT_UC_FAULT)
        != 0);
  }
  else {
    fault_status.iout = 0;
    fault_status.iout_oc_fault = 0;
    fault_status.iout_oc_warning = 0;
    fault_status.iout_uc_fault = 0;
  }

  if ((status_word & LTC2975_STATUS_INPUT) != 0) {
    fault_status.input = 1;
    // read STATUS_INPUT
    ltc2975_paged_read(ltc, LTC2975_CMD_STATUS_INPUT, page, rxdat, 1);
    fault_status.vin_ov_fault = ((rxdat[0] & LTC2975_STATUS_INPUT_VIN_OV_FAULT)
        != 0);
    fault_status.vin_ov_warning = ((rxdat[0]
        & LTC2975_STATUS_INPUT_VIN_OV_WARNING) != 0);
    fault_status.vin_uv_warning = ((rxdat[0]
        & LTC2975_STATUS_INPUT_VIN_UV_WARNING) != 0);
    fault_status.vin_uv_fault = ((rxdat[0] & LTC2975_STATUS_INPUT_VIN_UV_FAULT)
        != 0);
    fault_status.unit_off_for_insufficient_vin = ((rxdat[0]
        & LTC2975_STATUS_INPUT_UNIT_OFF_FOR_INSUFFICIENT_VIN) != 0);
  }
  else {
    fault_status.input = 0;
    fault_status.vin_ov_fault = 0;
    fault_status.vin_ov_warning = 0;
    fault_status.vin_uv_warning = 0;
    fault_status.vin_uv_fault = 0;
    fault_status.unit_off_for_insufficient_vin = 0;
  }

  if ((status_word & LTC2975_STATUS_MFR_SPECIFIC) != 0) {
    fault_status.mfr_specific = 1;
    // read STATUS_MFR_SPECIFIC
    ltc2975_paged_read(ltc, LTC2975_CMD_STATUS_MFR_SPECIFIC, page, rxdat, 1);
    fault_status.vout_discharge_fault = ((rxdat[0]
        & LTC2975_STATUS_MFRS_VOUT_DISCHARGE_FAULT) != 0);
    fault_status.fault1_in = ((rxdat[0] & LTC2975_STATUS_MFRS_FAULT1_IN) != 0);
    fault_status.fault0_in = ((rxdat[0] & LTC2975_STATUS_MFRS_FAULT0_IN) != 0);
    fault_status.watchdog_fault = ((rxdat[0]
        & LTC2975_STATUS_MFRS_WATCHDOG_FAULT) != 0);
  }
  else {
    fault_status.mfr_specific = 0;
    fault_status.vout_discharge_fault = 0;
    fault_status.fault1_in = 0;
    fault_status.fault0_in = 0;
    fault_status.watchdog_fault = 0;
  }

  if ((status_word & LTC2975_STATUS_POWER_NOT_GOOD) != 0) {
    fault_status.power_not_good = 1;
  }
  else {
    fault_status.power_not_good = 0;
  }

  if ((status_word & LTC2975_STATUS_BUSY) != 0) {
    fault_status.busy = 1;
  }
  else {
    fault_status.busy = 0;
  }

  if ((status_word & LTC2975_STATUS_OFF) != 0) {
    fault_status.off = 1;
  }
  else {
    fault_status.off = 0;
  }

  if ((status_word & LTC2975_STATUS_VOUT_OV) != 0) {
    fault_status.vout_ov = 1;
  }
  else {
    fault_status.vout_ov = 0;
  }

  if ((status_word & LTC2975_STATUS_IOUT_OC) != 0) {
    fault_status.iout_oc = 1;
  }
  else {
    fault_status.iout_oc = 0;
  }
  
  if ((status_word & LTC2975_STATUS_VIN_UV) != 0) {
    fault_status.vin_uv = 1;
  }
  else {
    fault_status.vin_uv = 0;
  }

  if ((status_word & LTC2975_STATUS_TEMPERATURE) != 0) {
    fault_status.temperature = 1;
    // read STATUS_TEMPERATURE
    ltc2975_paged_read(ltc, LTC2975_CMD_STATUS_TEMPERATURE, page, rxdat, 1);
    fault_status.temp_ot_fault = ((rxdat[0] & LTC2975_STATUS_TEMP_OT_FAULT) != 0);
    fault_status.temp_ot_warning =
        ((rxdat[0] & LTC2975_STATUS_TEMP_OT_WARNING) != 0);
    fault_status.temp_ut_warning =
        ((rxdat[0] & LTC2975_STATUS_TEMP_UT_WARNING) != 0);
    fault_status.temp_ut_fault = ((rxdat[0] & LTC2975_STATUS_TEMP_UT_FAULT) != 0);
  }
  else {
    fault_status.temperature = 0;
    fault_status.temp_ot_fault = 0;
    fault_status.temp_ot_warning = 0;
    fault_status.temp_ut_warning = 0;
    fault_status.temp_ut_fault = 0;
  }

  if ((status_word & LTC2975_STATUS_CML) != 0) {
    fault_status.cml = 1;
    // read STATUS_CML
    ltc2975_paged_read(ltc, LTC2975_CMD_STATUS_CML, page, rxdat, 1);
    fault_status.invalid_command = ((rxdat[0]
        & LTC2975_STATUS_CML_INVALID_COMMAND) != 0);
    fault_status.invalid_data = ((rxdat[0] & LTC2975_STATUS_CML_INVALID_DATA)
        != 0);
    fault_status.pec_failed = ((rxdat[0] & LTC2975_STATUS_CML_PEC_FAILED) != 0);
    fault_status.memory_fault = ((rxdat[0] & LTC2975_STATUS_CML_MEMORY_FAULT)
        != 0);
    fault_status.processor_fault = ((rxdat[0]
        & LTC2975_STATUS_CML_PROCESSOR_FAULT) != 0);
    fault_status.other_communication_fault = ((rxdat[0]
        & LTC2975_STATUS_CML_OTHER_COMMUNICATIONS_FAULT) != 0);
  }
  else {
    fault_status.cml = 0;
    fault_status.invalid_command = 0;
    fault_status.invalid_data = 0;
    fault_status.pec_failed = 0;
    fault_status.memory_fault = 0;
    fault_status.processor_fault = 0;
    fault_status.other_communication_fault = 0;
    fault_status.other_memory_or_logic_fault = 0;
  }

  if ((status_word & LTC2975_STATUS_OTHER) != 0) {
    fault_status.other = 1;
  }
  else {
    fault_status.other = 0;
  }

  return fault_status;
}

bool ltc2975_is_faulting(ltc2975_fault_status *fault_status) {
  return fault_status->vout || fault_status->iout || fault_status->input
      || fault_status->mfr_specific || fault_status->power_not_good
      || fault_status->busy || fault_status->off || fault_status->vout_ov
      || fault_status->iout_oc || fault_status->vout_uv
      || fault_status->temperature || fault_status->cml || fault_status->other;
}

//TODO deal with i2c errors - just assume chip is rebooting?
bool ltc2975_chip_busy(LTC2975 *ltc) {
  // The MFR_COMMON command has status bit 6 for the busy state
  uint8_t mfr_common[1];
  ltc2975_global_read(ltc, LTC2975_CMD_MFR_COMMON, mfr_common, 1);

  bool chip_not_busy = (mfr_common[0] >> LTC2975_MFRC_NOT_BUSY_BIT) & 0x01;

  return !chip_not_busy;
}

void ltc2975_wait_for_not_busy(LTC2975 *ltc) {
  while (ltc2975_chip_busy(ltc)) {
    chThdSleepMilliseconds(1);
  }
}

uint8_t ltc2975_check_comms(LTC2975 *ltc) {
  // Read MFR_SPECIAL_ID to check communication / ID part
  uint8_t rxdat[2];
  if (ltc2975_global_read(ltc, LTC2975_CMD_MFR_SPECIAL_ID, rxdat, 2) != ERR_OK) {
    return ERR_COMMS;
  }
  // We expect 0x0223
  if ((rxdat[0] == 0x02) && (rxdat[1] == 0x23)) {
    return ERR_OK;
  }
  return ERR_COMMS;
}

static uint8_t ltc2975_program_voltage(LTC2975 *ltc, uint8_t channel, float voltage,
                                       float fault_margin, float warn_margin){
  // TODO: I don't think this is necessary
  /*
  uint8_t data[2];
  uint16_t voltage_l16 = float_to_L16(l16_exp, voltage);
  data[0] = (voltage_l16 & 0xff);
  data[1] = (voltage_l16 >> 8) & 0xff;
  if (ltc2975_paged_write(ltc, LTC2975_CMD_VOUT_COMMAND, channel, data,
                          2) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc2975_wait_for_not_busy(ltc);
  */

  voltage_l16 = float_to_L16(l16_exp, voltage * (1 + fault_margin));
  data[0] = (voltage_l16 & 0xff);
  data[1] = (voltage_l16 >> 8) & 0xff;
  if (ltc2975_paged_write(ltc, LTC2975_CMD_VOUT_OV_FAULT_LIMIT, channel, data,
                          2) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc2975_wait_for_not_busy(ltc);
  
  voltage_l16 = float_to_L16(l16_exp, voltage * (1 + warn_margin));
  data[0] = (voltage_l16 & 0xff);
  data[1] = (voltage_l16 >> 8) & 0xff;
  if (ltc2975_paged_write(ltc, LTC2975_CMD_VOUT_OV_WARN_LIMIT, channel, data,
                          2) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc2975_wait_for_not_busy(ltc);

  voltage_l16 = float_to_L16(l16_exp, voltage * (1 - fault_margin));
  data[0] = (voltage_l16 & 0xff);
  data[1] = (voltage_l16 >> 8) & 0xff;
  if (ltc2975_paged_write(ltc, LTC2975_CMD_VOUT_UV_FAULT_LIMIT, channel, data,
                          2) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc2975_wait_for_not_busy(ltc);

  voltage_l16 = float_to_L16(l16_exp, voltage * (1 - warn_margin));
  data[0] = (voltage_l16 & 0xff);
  data[1] = (voltage_l16 >> 8) & 0xff;
  if (ltc2975_paged_write(ltc, LTC2975_CMD_VOUT_UV_WARN_LIMIT, channel, data,
                          2) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc2975_wait_for_not_busy(ltc);
  
  return ERR_OK;
}

uint8_t ltc2975_init(LTC2975 *ltc, I2CDriver *i2c, i2caddr_t address, float voltage,
                      const char *name1, float sense1_mohms, const char *name2, 
                      float sense2_mohms, const char *name3, float sense3_mohms,
                      const char *name4, float sense4_mohms) {

  // Fill out LTC2975 struct with provided information
  ltc->config.i2c = i2c;
  ltc->config.address = address;
  memcpy(ltc->config.name1, name1, strlen(name1) + 1);
  memcpy(ltc->config.name2, name2, strlen(name2) + 1);
  memcpy(ltc->config.name3, name3, strlen(name3) + 1);
  memcpy(ltc->config.name4, name4, strlen(name4) + 1);
  ltc->config.voltage = voltage;

  ltc->iout_1 = 0.0f;
  ltc->iout_2 = 0.0f;
  ltc->iout_3 = 0.0f;
  ltc->iout_4 = 0.0f;
  ltc->pout_1 = 0.0f;
  ltc->pout_2 = 0.0f;
  ltc->pout_3 = 0.0f;
  ltc->pout_4 = 0.0f;
  ltc->vout_1 = 0.0f;
  ltc->vout_2 = 0.0f;
  ltc->vout_3 = 0.0f;
  ltc->vout_4 = 0.0f;

  ltc->poll_time = 0;

  uint8_t data[4];

  // reset device
  if (ltc2975_global_write(ltc, LTC2975_CMD_MFR_RESET, data, 0) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc2975_wait_for_not_busy(ltc);

  // read exponent to use for l16-float conversions
  if (ltc2975_read_l16_exp(ltc) != ERR_OK) {
    return ERR_COMMS;
  }

  // set on_off_config to respond to PMbus OPERATION command
  data[0] = 0x1E | (1 << LTC2975_ONOFF_USE_PMBUS); // default value is 0x1E
  if (ltc2975_global_write(ltc, LTC2975_CMD_ON_OFF_CONFIG, data, 1) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc2975_wait_for_not_busy(ltc);

  // set voltages as per *ltc
  if(ltc2975_program_voltage(ltc, PAGE_0, voltage1, 0.05) != ERR_OK){
    return ERR_COMMS;
  }
  if(ltc2975_program_voltage(ltc, PAGE_1, voltage2, 0.05) != ERR_OK){
    return ERR_COMMS;
  }

  // set frequency to 575kHz
  data[0] = (LTC2975_FREQ_575KHZ & 0xff);
  data[1] = (LTC2975_FREQ_575KHZ >> 8) & 0xff;
  if (ltc2975_global_write(ltc, LTC2975_CMD_FREQENCY_SWITCH, data, 2) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc2975_wait_for_not_busy(ltc);

  // set sense resistor value to 50mOhms
  uint16_t iout_cal_gain = float_to_L11(sense1_mohms);
  data[0] = iout_cal_gain & 0xff;
  data[1] = (iout_cal_gain >> 8) & 0xff;
  if (ltc2975_paged_write(ltc, LTC2975_CMD_IOUT_CAL_GAIN, PAGE_0, data, 2) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc2975_wait_for_not_busy(ltc);

  iout_cal_gain = float_to_L11(sense2_mohms);
  data[0] = iout_cal_gain & 0xff;
  data[1] = (iout_cal_gain >> 8) & 0xff;
  if (ltc2975_paged_write(ltc, LTC2975_CMD_IOUT_CAL_GAIN, PAGE_1, data, 2) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc2975_wait_for_not_busy(ltc);

  // disable temperature sensing and alert
  // See UT_FAULT_LIMIT, UT_FAULT_RESPONSE commands
  data[0] = (LTC2975_UT_LIMIT_MIN & 0xff);
  data[1] = (LTC2975_UT_LIMIT_MIN >> 8) & 0xff;
  if (ltc2975_global_write(ltc, LTC2975_CMD_UT_FAULT_LIMIT, data, 2) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc2975_wait_for_not_busy(ltc);

  data[0] = LTC2975_UT_FAULT_IGNORE;
  if (ltc2975_global_write(ltc, LTC2975_CMD_UT_FAULT_RESPONSE, data,
                           1) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc2975_wait_for_not_busy(ltc);

  // Mask UT_FAULT alert
  data[0] = LTC2975_CMD_STATUS_TEMPERATURE;
  data[1] = (1 << LTC2975_FAULT_EXT_UT_BIT);
  if (ltc2975_global_write(ltc, LTC2975_CMD_SMBALERT_MASK, data, 2) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc2975_wait_for_not_busy(ltc);

  // Disable Share Clock Control
  // XXX Also disable GPIO Alerts
  data[0] = 0x1D & ~(1 << LTC2975_SHARE_CLK_CONTROL);
  data[0] &= ~(1 << LTC2975_NO_GPIO_ALERT);
  if (ltc2975_global_write(ltc, LTC2975_CMD_MFR_CHAN_CONFIG_LTC2975, data, 1) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc2975_wait_for_not_busy(ltc);

  // Ignore GPIO (since nothing can pull it down externally anyway)
  data[0] = 0x00;
  if (ltc2975_global_write(ltc, LTC2975_CMD_MFR_GPIO_RESPONSE, data, 1) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc2975_wait_for_not_busy(ltc);

  // Clear Faults
  ltc2975_clear_faults(ltc);

  return ERR_OK;
}

uint8_t ltc2975_poll(LTC2975 *ltc) {
  uint8_t rxdat[2];

  // Read voltage of both outputs
  if (ltc2975_paged_read(ltc, LTC2975_CMD_READ_VOUT, PAGE_0, rxdat,
                         sizeof(rxdat)) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc->vout_1 = L16_to_float(l16_exp, ((rxdat[1] << 8) | rxdat[0]));
  if (ltc2975_paged_read(ltc, LTC2975_CMD_READ_VOUT, PAGE_1, rxdat,
                         sizeof(rxdat)) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc->vout_2 = L16_to_float(l16_exp, ((rxdat[1] << 8) | rxdat[0]));

  // Read current of both outputs
  if (ltc2975_paged_read(ltc, LTC2975_CMD_READ_IOUT, PAGE_0, rxdat,
                         sizeof(rxdat)) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc->iout_1 = L11_to_float((rxdat[1] << 8) | rxdat[0]);
  if (ltc2975_paged_read(ltc, LTC2975_CMD_READ_IOUT, PAGE_1, rxdat,
                         sizeof(rxdat)) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc->iout_2 = L11_to_float((rxdat[1] << 8) | rxdat[0]);

  // Read power of both outputs, or calculate it (faster)
#if LTC2975_READ_CALCULATED_POWER
  if (ltc2975_paged_read(ltc, LTC2975_CMD_READ_POUT, PAGE_0, rxdat,
                         sizeof(rxdat)) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc->pout_1 = L11_to_float((rxdat[1] << 8) | rxdat[0]);
  if (ltc2975_paged_read(ltc, LTC2975_CMD_READ_POUT, PAGE_1, rxdat,
                         sizeof(rxdat)) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc->pout_2 = L11_to_float((rxdat[1] << 8) | rxdat[0]);
#else
  ltc->pout_1 = ltc->vout_1 * ltc->iout_1;
  ltc->pout_2 = ltc->vout_2 * ltc->iout_2;
#endif /* LTC2975_READ_CALCULATED_POWER */

  // Track the last polled time to notice stale data
  ltc->poll_time = chVTGetSystemTime();

  return ERR_OK;
}

uint8_t ltc2975_turn_on(LTC2975 *ltc, uint8_t channel) {
  uint8_t data[1];
  data[0] = LTC2975_OPER_ON;
  if (ltc2975_paged_write(ltc, LTC2975_CMD_OPERATION, channel, data,
                         1) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc2975_wait_for_not_busy(ltc);

  return ERR_OK;
}

uint8_t ltc2975_turn_off(LTC2975 *ltc, uint8_t channel) {
  uint8_t data[1];
  data[0] = LTC2975_OPER_OFF;
  if (ltc2975_paged_write(ltc, LTC2975_CMD_OPERATION, channel, data,
                         1) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc2975_wait_for_not_busy(ltc);
  return ERR_OK;
}
