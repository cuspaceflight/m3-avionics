/*
 * LTC3887 Driver
 * Cambridge University Spaceflight
 */

#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "config.h"
#include "ltc3887.h"
#include "smbus.h"
#include "pmbus_util.h"

/* Command codes */
#define LTC3887_CMD_OPERATION           0x01
#define LTC3887_CMD_ON_OFF_CONFIG       0x02
#define LTC3887_CMD_CLEAR_FAULTS        0x03
#define LTC3887_CMD_PAGE_PLUS_WRITE     0x05
#define LTC3887_CMD_PAGE_PLUS_READ      0x06
#define LTC3887_CMD_SMBALERT_MASK       0x1B
#define LTC3887_CMD_VOUT_MODE           0x20
#define LTC3887_CMD_VOUT_COMMAND        0x21
#define LTC3887_CMD_FREQENCY_SWITCH     0x33
#define LTC3887_CMD_UT_FAULT_LIMIT      0x53
#define LTC3887_CMD_UT_FAULT_RESPONSE   0x54
#define LTC3887_CMD_STATUS_WORD         0x79
#define LTC3887_CMD_STATUS_VOUT         0x7A
#define LTC3887_CMD_STATUS_IOUT         0x7B
#define LTC3887_CMD_STATUS_INPUT        0x7C
#define LTC3887_CMD_STATUS_TEMPERATURE  0x7D
#define LTC3887_CMD_STATUS_CML          0x7E
#define LTC3887_CMD_STATUS_MFR_SPECIFIC 0x80
#define LTC3887_CMD_READ_VOUT           0x8B
#define LTC3887_CMD_READ_IOUT           0x8C
#define LTC3887_CMD_READ_POUT           0x96
#define LTC3887_CMD_MFR_SPECIAL_ID      0xE7
#define LTC3887_CMD_MFR_COMMON          0xEF

/* Register descriptions */
#define LTC3887_MFRC_ONIT_BIT       4 /* Output not in transition */
#define LTC3887_MFRC_CNP_BIT        5 /* Calculations not pending */
#define LTC3887_MFRC_CNB_BIT        6 /* Chip not busy */
#define LTC3887_MFRC_CNA_BIT        7 /* Chip not ALERTing */

#define LTC3887_STATUS_VOUT         (1<<15)
#define LTC3887_STATUS_IOUT         (1<<14)
#define LTC3887_STATUS_INPUT        (1<<13)
#define LTC3887_STATUS_MFR_SPECIFIC (1<<12)
#define LTC3887_STATUS_POWER_GOOD   (1<<11)
#define LTC3887_STATUS_BUSY         (1<<7)
#define LTC3887_STATUS_OFF          (1<<6)
#define LTC3887_STATUS_VOUT_OV      (1<<5)
#define LTC3887_STATUS_IOUT_OC      (1<<4)
#define LTC3887_STATUS_TEMPERATURE  (1<<2)
#define LTC3887_STATUS_CML          (1<<1)
#define LTC3887_STATUS_OTHER        (1<<0)

#define LTC3887_STATUS_VOUT_VOUT_OV_FAULT       (1<<7)
#define LTC3887_STATUS_VOUT_VOUT_OV_WARNING     (1<<6)
#define LTC3887_STATUS_VOUT_VOUT_UV_WARNING     (1<<5)
#define LTC3887_STATUS_VOUT_VOUT_UV_FAULT       (1<<4)
#define LTC3887_STATUS_VOUT_VOUT_MAX_WARNING    (1<<3)
#define LTC3887_STATUS_VOUT_TON_MAX_FAULT       (1<<2)
#define LTC3887_STATUS_VOUT_TOFF_MAX_WARNING    (1<<1)
#define LTC3887_STATUS_IOUT_IOUT_OC_FAULT       (1<<7)
#define LTC3887_STATUS_IOUT_IOUT_OC_WARNING     (1<<6)
#define LTC3887_STATUS_INPUT_VIN_OV_FAULT       (1<<7)
#define LTC3887_STATUS_INPUT_VIN_UV_WARNING     (1<<5)
#define LTC3887_STATUS_INPUT_UNIT_OFF_FOR_INSUFFICIENT_VIN (1<<3)
#define LTC3887_STATUS_INPUT_IIN_OC_WARNING     (1<<1)
#define LTC3887_STATUS_MFRS_INTERNAL_TEMP_FAULT (1<<7)
#define LTC3887_STATUS_MFRS_INTERNAL_TEMP_WARNING (1<<6)
#define LTC3887_STATUS_MFRS_EEPROM_CRC_ERROR    (1<<5)
#define LTC3887_STATUS_MFRS_INTERNAL_PLL_UNLOCKED (1<<4)
#define LTC3887_STATUS_MFRS_FAULT_LOG_PRESENT   (1<<3)
#define LTC3887_STATUS_MFRS_VOUT_SHORT_CYCLED   (1<<1)
#define LTC3887_STATUS_MFRS_GPIO_PULLED_LOW_EXTERNALLY (1<<0)
#define LTC3887_STATUS_TEMP_OT_FAULT            (1<<7)
#define LTC3887_STATUS_TEMP_OT_WARNING          (1<<6)
#define LTC3887_STATUS_TEMP_UT_FAULT            (1<<4)
#define LTC3887_STATUS_CML_INVALID_COMMAND      (1<<7)
#define LTC3887_STATUS_CML_INVALID_DATA         (1<<6)
#define LTC3887_STATUS_CML_PEC_FAILED           (1<<5)
#define LTC3887_STATUS_CML_MEMORY_FAULT         (1<<4)
#define LTC3887_STATUS_CML_PROCESSOR_FAULT      (1<<3)
#define LTC3887_STATUS_CML_OTHER_COMMUNICATIONS_FAULT (1<<1)
#define LTC3887_STATUS_CML_OTHER_MEMORY_OR_LOGIC_FAULT (1<<0)

#define LTC3887_ONOFF_USE_PMBUS     3 /* Allow on/off control via I2C */

#define LTC3887_OPER_ON             0x80 /* Turn on channel */
#define LTC3887_OPER_OFF            0x40 /* Turn off channel */

#define LTC3887_FREQ_425KHZ         0xFB52 /* Switch at 425kHz */

#define LTC3887_UT_LIMIT_MIN        0xFDDA /* 0xFDDA = -275C (disable UT warning) */
#define LTC3887_UT_FAULT_IGNORE     0x00 /* Ignore under-temperature faults */

/* Fault bits */
#define LTC3887_FAULT_EXT_UT_BIT    4

/* PAGE constants */
#define PAGE_0      0x00
#define PAGE_1      0x01
#define PAGE_BOTH   0xFF

static int8_t l16_exp;

uint8_t ltc3887_paged_write(LTC3887 *ltc, uint8_t command_code, uint8_t page, uint8_t *data, uint8_t datalen) {
  uint8_t txdat[datalen + 4];

  txdat[0] = page;
  txdat[1] = command_code;

  // copy rest of data
  memcpy(&txdat[4], data, datalen);
  return smbus_write_block(ltc->config.i2c, ltc->config.address, LTC3887_CMD_PAGE_PLUS_WRITE, txdat, datalen+4);
}

uint8_t ltc3887_paged_read(LTC3887 *ltc, uint8_t command_code, uint8_t page,
                           uint8_t *data, uint8_t datalen) {
  uint8_t txdat[2];
  txdat[0] = page;
  txdat[1] = command_code;

  return smbus_read_block(ltc->config.i2c, ltc->config.address, LTC3887_CMD_PAGE_PLUS_READ, txdat, sizeof(txdat), data, datalen);
}

uint8_t ltc3887_global_write(LTC3887 *ltc, uint8_t command_code, uint8_t *data,
                             uint8_t datalen) {
  return ltc3887_paged_write(ltc, command_code, PAGE_BOTH, data, datalen);
}

uint8_t ltc3887_global_read(LTC3887 *ltc, uint8_t command_code, uint8_t *data,
                            uint8_t datalen) {
  return ltc3887_paged_read(ltc, command_code, PAGE_BOTH, data, datalen);
}

uint8_t ltc3887_read_l16_exp(LTC3887 *ltc) {
  // Read the VOUT_MODE register to get L16 exponent to use
  uint8_t rxdat[1];
  uint8_t status = ltc3887_global_read(ltc, LTC3887_CMD_VOUT_MODE, rxdat,
                                       sizeof(rxdat));

  // sign-extend 5 bits
  if (rxdat[0] > 0x0F)
    rxdat[0] |= 0xE0;
  l16_exp = rxdat[0];

  return status;
}

bool ltc3887_is_alerting(LTC3887 *ltc) {
  uint8_t mfr_common[1];
  ltc3887_global_read(ltc, LTC3887_CMD_MFR_COMMON, mfr_common,
                      sizeof(mfr_common));

  if ((mfr_common[0] & (1 << LTC3887_MFRC_CNA_BIT)) != 0) {
    return TRUE;
  }
  return FALSE;
}

uint8_t ltc3887_clear_faults(LTC3887 *ltc) {
  // Send a CLEAR_FAULTS command and wait for it to complete
  if (ltc3887_global_write(ltc, LTC3887_CMD_CLEAR_FAULTS, NULL, 0) != ERR_OK) {
    return ERR_COMMS;
  }
  // It can take up to 10us to complete (no way of checking?)
  chThdSleepMicroseconds(10);
  return ERR_OK;
}

ltc3887_fault_status ltc3887_get_fault_status(LTC3887 *ltc, uint8_t page) {
  uint8_t rxdat[2];

  // Read STATUS_WORD to get overview of where faults are
  ltc3887_paged_read(ltc, LTC3887_CMD_STATUS_WORD, page, rxdat, sizeof(rxdat));
  uint16_t status_word = (rxdat[0] << 8) | rxdat[1];

  ltc3887_fault_status fault_status;

  if ((status_word & LTC3887_STATUS_VOUT) != 0) {
    fault_status.vout = 1;
    // read STATUS_VOUT
    ltc3887_paged_read(ltc, LTC3887_CMD_STATUS_VOUT, page, rxdat, 1);
    fault_status.vout_ov_fault = ((rxdat[0] & LTC3887_STATUS_VOUT_VOUT_OV_FAULT)
        != 0);
    fault_status.vout_ov_warning = ((rxdat[0]
        & LTC3887_STATUS_VOUT_VOUT_OV_WARNING) != 0);
    fault_status.vout_uv_warning = ((rxdat[0]
        & LTC3887_STATUS_VOUT_VOUT_UV_WARNING) != 0);
    fault_status.vout_uv_fault = ((rxdat[0] & LTC3887_STATUS_VOUT_VOUT_UV_FAULT)
        != 0);
    fault_status.vout_max_warning = ((rxdat[0]
        & LTC3887_STATUS_VOUT_VOUT_MAX_WARNING) != 0);
    fault_status.ton_max_fault = ((rxdat[0] & LTC3887_STATUS_VOUT_TON_MAX_FAULT)
        != 0);
    fault_status.toff_max_warning = ((rxdat[0]
        & LTC3887_STATUS_VOUT_TOFF_MAX_WARNING) != 0);
  }
  else {
    fault_status.vout = 0;
    fault_status.vout_ov_fault = 0;
    fault_status.vout_ov_warning = 0;
    fault_status.vout_uv_warning = 0;
    fault_status.vout_uv_fault = 0;
    fault_status.vout_max_warning = 0;
    fault_status.ton_max_fault = 0;
    fault_status.toff_max_warning = 0;
  }

  if ((status_word & LTC3887_STATUS_IOUT) != 0) {
    fault_status.iout = 1;
    // read STATUS_IOUT
    ltc3887_paged_read(ltc, LTC3887_CMD_STATUS_IOUT, page, rxdat, 1);
    fault_status.iout_oc_fault = ((rxdat[0] & LTC3887_STATUS_IOUT_IOUT_OC_FAULT)
        != 0);
    fault_status.iout_oc_warning = ((rxdat[0]
        & LTC3887_STATUS_IOUT_IOUT_OC_WARNING) != 0);
  }
  else {
    fault_status.iout = 0;
    fault_status.iout_oc_fault = 0;
    fault_status.iout_oc_warning = 0;
  }

  if ((status_word & LTC3887_STATUS_INPUT) != 0) {
    fault_status.input = 1;
    // read STATUS_INPUT
    ltc3887_paged_read(ltc, LTC3887_CMD_STATUS_INPUT, page, rxdat, 1);
    fault_status.vin_ov_fault = ((rxdat[0] & LTC3887_STATUS_INPUT_VIN_OV_FAULT)
        != 0);
    fault_status.vin_uv_warning = ((rxdat[0]
        & LTC3887_STATUS_INPUT_VIN_UV_WARNING) != 0);
    fault_status.unit_off_for_insufficient_vin = ((rxdat[0]
        & LTC3887_STATUS_INPUT_UNIT_OFF_FOR_INSUFFICIENT_VIN) != 0);
    fault_status.iin_oc_warning = ((rxdat[0]
        & LTC3887_STATUS_INPUT_IIN_OC_WARNING) != 0);
  }
  else {
    fault_status.input = 0;
    fault_status.vin_ov_fault = 0;
    fault_status.vin_uv_warning = 0;
    fault_status.unit_off_for_insufficient_vin = 0;
    fault_status.iin_oc_warning = 0;
  }

  if ((status_word & LTC3887_STATUS_MFR_SPECIFIC) != 0) {
    fault_status.mfr_specific = 1;
    // read STATUS_MFR_SPECIFIC
    ltc3887_paged_read(ltc, LTC3887_CMD_STATUS_MFR_SPECIFIC, page, rxdat, 1);
    fault_status.internal_temperature_fault = ((rxdat[0]
        & LTC3887_STATUS_MFRS_INTERNAL_TEMP_FAULT) != 0);
    fault_status.internal_temperature_warning = ((rxdat[0]
        & LTC3887_STATUS_MFRS_INTERNAL_TEMP_WARNING) != 0);
    fault_status.eeprom_crc_error = ((rxdat[0]
        & LTC3887_STATUS_MFRS_EEPROM_CRC_ERROR) != 0);
    fault_status.internal_pll_unlocked = ((rxdat[0]
        & LTC3887_STATUS_MFRS_INTERNAL_PLL_UNLOCKED) != 0);
    fault_status.fault_log_present = ((rxdat[0]
        & LTC3887_STATUS_MFRS_FAULT_LOG_PRESENT) != 0);
    fault_status.vout_short_cycled = ((rxdat[0]
        & LTC3887_STATUS_MFRS_VOUT_SHORT_CYCLED) != 0);
    fault_status.gpio_pulled_low_externally = ((rxdat[0]
        & LTC3887_STATUS_MFRS_GPIO_PULLED_LOW_EXTERNALLY) != 0);
  }
  else {
    fault_status.mfr_specific = 0;
    fault_status.internal_temperature_fault = 0;
    fault_status.internal_temperature_warning = 0;
    fault_status.eeprom_crc_error = 0;
    fault_status.internal_pll_unlocked = 0;
    fault_status.fault_log_present = 0;
    fault_status.vout_short_cycled = 0;
    fault_status.gpio_pulled_low_externally = 0;
  }

  if ((status_word & LTC3887_STATUS_POWER_GOOD) != 0) {
    fault_status.power_good = 0;
  }
  else {
    fault_status.power_good = 1;
  }

  if ((status_word & LTC3887_STATUS_BUSY) != 0) {
    fault_status.busy = 1;
  }
  else {
    fault_status.busy = 0;
  }

  if ((status_word & LTC3887_STATUS_OFF) != 0) {
    fault_status.off = 1;
  }
  else {
    fault_status.off = 0;
  }

  if ((status_word & LTC3887_STATUS_VOUT_OV) != 0) {
    fault_status.vout_ov = 1;
  }
  else {
    fault_status.vout_ov = 0;
  }

  if ((status_word & LTC3887_STATUS_IOUT_OC) != 0) {
    fault_status.iout_oc = 1;
  }
  else {
    fault_status.iout_oc = 0;
  }

  if ((status_word & LTC3887_STATUS_TEMPERATURE) != 0) {
    fault_status.temperature = 1;
    // read STATUS_TEMPERATURE
    ltc3887_paged_read(ltc, LTC3887_CMD_STATUS_MFR_SPECIFIC, page, rxdat, 1);
    fault_status.ot_fault = ((rxdat[0] & LTC3887_STATUS_TEMP_OT_FAULT) != 0);
    fault_status.ot_warning =
        ((rxdat[0] & LTC3887_STATUS_TEMP_OT_WARNING) != 0);
    fault_status.ut_fault = ((rxdat[0] & LTC3887_STATUS_TEMP_UT_FAULT) != 0);
  }
  else {
    fault_status.temperature = 0;
    fault_status.ot_fault = 0;
    fault_status.ot_warning = 0;
    fault_status.ut_fault = 0;
  }

  if ((status_word & LTC3887_STATUS_CML) != 0) {
    fault_status.cml = 1;
    // read STATUS_CML
    ltc3887_paged_read(ltc, LTC3887_CMD_STATUS_MFR_SPECIFIC, page, rxdat, 1);
    fault_status.invalid_command = ((rxdat[0]
        & LTC3887_STATUS_CML_INVALID_COMMAND) != 0);
    fault_status.invalid_data = ((rxdat[0] & LTC3887_STATUS_CML_INVALID_DATA)
        != 0);
    fault_status.pec_failed = ((rxdat[0] & LTC3887_STATUS_CML_PEC_FAILED) != 0);
    fault_status.memory_fault = ((rxdat[0] & LTC3887_STATUS_CML_MEMORY_FAULT)
        != 0);
    fault_status.processor_fault = ((rxdat[0]
        & LTC3887_STATUS_CML_PROCESSOR_FAULT) != 0);
    fault_status.other_communication_fault = ((rxdat[0]
        & LTC3887_STATUS_CML_OTHER_COMMUNICATIONS_FAULT) != 0);
    fault_status.other_memory_or_logic_fault = ((rxdat[0]
        & LTC3887_STATUS_CML_OTHER_MEMORY_OR_LOGIC_FAULT) != 0);
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

  if ((status_word && LTC3887_STATUS_OTHER) != 0) {
    fault_status.other = 1;
  }
  else {
    fault_status.other = 0;
  }

  return fault_status;
}

bool ltc3887_is_faulting(ltc3887_fault_status *fault_status) {
  return fault_status->vout || fault_status->iout || fault_status->input
      || fault_status->mfr_specific || fault_status->power_good
      || fault_status->busy || fault_status->off || fault_status->vout_ov
      || fault_status->iout_oc || fault_status->temperature || fault_status->cml
      || fault_status->other;
}

ltc3887_busy_status ltc3887_chip_busy_status(LTC3887 *ltc) {
  // The MFR_COMMON command has status bits 4-6 for the busy state
  uint8_t mfr_common[1];
  ltc3887_global_read(ltc, LTC3887_CMD_MFR_COMMON, mfr_common, 1);

  bool output_not_in_transition = (mfr_common[0] >> LTC3887_MFRC_ONIT_BIT)
      & 0x01;
  bool calculations_not_pending = (mfr_common[0] >> LTC3887_MFRC_CNP_BIT)
      & 0x01;
  bool chip_not_busy = (mfr_common[0] >> LTC3887_MFRC_CNB_BIT) & 0x01;

  if (!chip_not_busy) {
    return CHIP_BUSY;
  }
  if (!calculations_not_pending) {
    return CALCULATIONS_PENDING;
  }
  if (!output_not_in_transition) {
    return OUTPUT_IN_TRANSITION;
  }
  return NOT_BUSY;
}

void ltc3887_wait_for_not_busy(LTC3887 *ltc) {
  while (ltc3887_chip_busy_status(ltc) != NOT_BUSY) {
    chThdSleepMilliseconds(1);
  }
}

uint8_t ltc3887_check_comms(LTC3887 *ltc) {
  // Read MFR_SPECIAL_ID to check communication / ID part
  uint8_t rxdat[2];
  if (ltc3887_global_read(ltc, LTC3887_CMD_MFR_SPECIAL_ID, rxdat, 2) != ERR_OK) {
    return ERR_COMMS;
  }
  // We expect 0x470X, where X can be changed by manufacturer
  if (rxdat[0] == 0x47 && ((rxdat[1] && 0xf0) == 0x00)) {
    return ERR_OK;
  }
  return ERR_COMMS;
}

uint8_t ltc3887_init(LTC3887 *ltc, I2CDriver *i2c, i2caddr_t address,
                     char name1[], float voltage1, char name2[], float voltage2) {

  // Fill out LTC3887 struct with provided information
  ltc->config.i2c = i2c;
  ltc->config.address = address;
  memcpy(name1, ltc->config.name1, strlen(name1) + 1);
  memcpy(name2, ltc->config.name2, strlen(name2) + 1);
  ltc->config.voltage1 = voltage1;
  ltc->config.voltage2 = voltage2;

  ltc->iout_1 = 0.0f;
  ltc->iout_2 = 0.0f;
  ltc->pout_1 = 0.0f;
  ltc->pout_2 = 0.0f;
  ltc->vout_1 = 0.0f;
  ltc->vout_2 = 0.0f;

  ltc->poll_time = 0;

  uint8_t data[4];

  // read exponent to use for l16-float conversions
  if (ltc3887_read_l16_exp(ltc) != ERR_OK) {
    return ERR_COMMS;
  }

  // set on_off_config to respond to PMbus OPERATION command
  data[0] = 0x1E | (1 << LTC3887_ONOFF_USE_PMBUS); // default value is 0x1E
  if (ltc3887_global_write(ltc, LTC3887_CMD_ON_OFF_CONFIG, data, 1) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc3887_wait_for_not_busy(ltc);

  // set voltages as per *ltc
  uint16_t voltage_l16 = float_to_L16(l16_exp, voltage1);
  data[0] = (voltage_l16 >> 8) & 0xff;
  data[1] = (voltage_l16 & 0xff);
  if (ltc3887_paged_write(ltc, LTC3887_CMD_VOUT_COMMAND, PAGE_0, data,
                          2) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc3887_wait_for_not_busy(ltc);

  voltage_l16 = float_to_L16(l16_exp, voltage2);
  data[0] = (voltage_l16 >> 8) & 0xff;
  data[1] = (voltage_l16 & 0xff);
  if (ltc3887_paged_write(ltc, LTC3887_CMD_VOUT_COMMAND, PAGE_1, data,
                          2) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc3887_wait_for_not_busy(ltc);

  // set frequency to 425kHz
  data[0] = (LTC3887_FREQ_425KHZ >> 8) & 0xff;
  data[1] = (LTC3887_FREQ_425KHZ & 0xff);
  if (ltc3887_global_write(ltc, LTC3887_CMD_FREQENCY_SWITCH, data, 2) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc3887_wait_for_not_busy(ltc);

  // disable temperature sensing and alert
  // See UT_FAULT_LIMIT, UT_FAULT_RESPONSE commands
  data[0] = (LTC3887_UT_LIMIT_MIN >> 8) & 0xff;
  data[1] = (LTC3887_UT_LIMIT_MIN & 0xff);
  if (ltc3887_global_write(ltc, LTC3887_CMD_UT_FAULT_LIMIT, data, 2) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc3887_wait_for_not_busy(ltc);

  data[0] = LTC3887_UT_FAULT_IGNORE;
  if (ltc3887_global_write(ltc, LTC3887_CMD_UT_FAULT_RESPONSE, data,
                           1) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc3887_wait_for_not_busy(ltc);

  // Mask UT_FAULT alert
  data[0] = LTC3887_CMD_STATUS_TEMPERATURE;
  data[1] = (1 << LTC3887_FAULT_EXT_UT_BIT);
  if (ltc3887_global_write(ltc, LTC3887_CMD_SMBALERT_MASK, data, 2) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc3887_wait_for_not_busy(ltc);

  return ERR_OK;
}

uint8_t ltc3887_poll(LTC3887 *ltc) {
  uint8_t rxdat[2];

  // Read voltage of both outputs
  if (ltc3887_paged_read(ltc, LTC3887_CMD_READ_VOUT, PAGE_0, rxdat,
                         sizeof(rxdat)) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc->vout_1 = L16_to_float(l16_exp, ((rxdat[0] << 8) | rxdat[1]));
  if (ltc3887_paged_read(ltc, LTC3887_CMD_READ_VOUT, PAGE_1, rxdat,
                         sizeof(rxdat)) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc->vout_2 = L16_to_float(l16_exp, ((rxdat[0] << 8) | rxdat[1]));

  // Read current of both outputs
  if (ltc3887_paged_read(ltc, LTC3887_CMD_READ_IOUT, PAGE_0, rxdat,
                         sizeof(rxdat)) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc->iout_1 = L16_to_float(l16_exp, ((rxdat[0] << 8) | rxdat[1]));
  if (ltc3887_paged_read(ltc, LTC3887_CMD_READ_IOUT, PAGE_1, rxdat,
                         sizeof(rxdat)) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc->iout_2 = L16_to_float(l16_exp, ((rxdat[0] << 8) | rxdat[1]));

  // Read power of both outputs, or calculate it (faster)
#if LTC3887_READ_CALCULATED_POWER
  if (ltc3887_paged_read(ltc, LTC3887_CMD_READ_POUT, PAGE_0, rxdat,
                         sizeof(rxdat)) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc->pout_1 = L16_to_float(l16_exp, ((rxdat[0] << 8) | rxdat[1]));
  if (ltc3887_paged_read(ltc, LTC3887_CMD_READ_POUT, PAGE_1, rxdat,
                         sizeof(rxdat)) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc->pout_2 = L16_to_float(l16_exp, ((rxdat[0] << 8) | rxdat[1]));
#else
  ltc->pout_1 = ltc->vout_1 * ltc->iout_1;
  ltc->pout_2 = ltc->vout_2 * ltc->iout_2;
#endif /* LTC3887_READ_CALCULATED_POWER */

  // Track the last polled time to notice stale data
  ltc->poll_time = chVTGetSystemTime();

  return ERR_OK;
}

uint8_t ltc3887_turn_on(LTC3887 *ltc, uint8_t channel) {
  uint8_t data[1];
  data[0] = LTC3887_OPER_ON;
  if (ltc3887_paged_write(ltc, LTC3887_CMD_OPERATION, channel, data,
                         1) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc3887_wait_for_not_busy(ltc);
  return ERR_OK;
}

uint8_t ltc3887_turn_off(LTC3887 *ltc, uint8_t channel) {
  uint8_t data[1];
  data[0] = LTC3887_OPER_OFF;
  if (ltc3887_paged_write(ltc, LTC3887_CMD_OPERATION, channel, data,
                         1) != ERR_OK) {
    return ERR_COMMS;
  }
  ltc3887_wait_for_not_busy(ltc);
  return ERR_OK;
}
