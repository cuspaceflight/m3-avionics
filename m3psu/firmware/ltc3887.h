/*
 * LTC3887 Driver
 * Cambridge University Spaceflight
 */


#ifndef LTC3887_H_
#define LTC3887_H_

#include "ch.h"
#include "hal.h"

typedef struct {
  I2CDriver *i2c;
  i2caddr_t address;
  char name1[12];
  float voltage1;
  char name2[12];
  float voltage2;
} LTC3887Config;

typedef struct {
  LTC3887Config config;
  float vout_1;
  float iout_1;
  float pout_1;
  float vout_2;
  float iout_2;
  float pout_2;
  systime_t poll_time;
} LTC3887;

typedef enum {
  NOT_BUSY,
  OUTPUT_IN_TRANSITION,
  CALCULATIONS_PENDING,
  CHIP_BUSY
} ltc3887_busy_status;

typedef struct {
  // summary flags
  bool vout:1;
  bool iout:1;
  bool input:1;
  bool mfr_specific:1;
  bool power_not_good:1;
  bool busy:1;
  bool off:1;
  bool vout_ov:1;
  bool iout_oc:1;
  bool temperature:1;
  bool cml:1;
  bool other:1;
  // specific faults
  bool vout_ov_fault:1;
  bool vout_ov_warning:1;
  bool vout_uv_warning:1;
  bool vout_uv_fault:1;
  bool vout_max_warning:1;
  bool ton_max_fault:1;
  bool toff_max_warning:1;
  bool iout_oc_fault:1;
  bool iout_oc_warning:1;
  bool vin_ov_fault:1;
  bool vin_uv_warning:1;
  bool unit_off_for_insufficient_vin:1;
  bool iin_oc_warning:1;
  bool internal_temperature_fault:1;
  bool internal_temperature_warning:1;
  bool eeprom_crc_error:1;
  bool internal_pll_unlocked:1;
  bool fault_log_present:1;
  bool vout_short_cycled:1;
  bool gpio_pulled_low_externally:1;
  bool ot_fault:1;
  bool ot_warning:1;
  bool ut_fault:1;
  bool invalid_command:1;
  bool invalid_data:1;
  bool pec_failed:1;
  bool memory_fault:1;
  bool processor_fault:1;
  bool other_communication_fault:1;
  bool other_memory_or_logic_fault:1;
} ltc3887_fault_status;

uint8_t ltc3887_init(LTC3887 *ltc, I2CDriver *i2c, i2caddr_t address, const char *name1, float voltage1, float sense1_mohms,
                      const char *name2, float voltage2, float sense2_mohms);
bool ltc3887_is_alerting(LTC3887 *ltc);
uint8_t ltc3887_clear_faults(LTC3887 *ltc);
ltc3887_fault_status ltc3887_get_fault_status(LTC3887 *ltc, uint8_t page);
bool ltc3887_is_faulting(ltc3887_fault_status *fault_status);
uint8_t ltc3887_check_comms(LTC3887 *ltc);
uint8_t ltc3887_poll(LTC3887 *ltc);
uint8_t ltc3887_turn_on(LTC3887 *ltc, uint8_t channel);
uint8_t ltc3887_turn_off(LTC3887 *ltc, uint8_t channel);

#endif /* LTC3887_H_ */
