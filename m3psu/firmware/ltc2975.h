/*
 * LTC2975 Driver
 * Cambridge University Spaceflight
 */


#ifndef LTC2975_H_
#define LTC2975_H_

#include "ch.h"
#include "hal.h"

typedef struct {
  I2CDriver *i2c;
  i2caddr_t address;
  float voltage1;
  float voltage2;
  float voltage3;
  float voltage4;
  char name1[12];
  char name2[12];
  char name3[12];
  char name4[12];
} LTC2975Config;

typedef struct {
  LTC2975Config config;
  float vout[4];
  float iout[4];
  float pout[4];
  systime_t poll_time;
} LTC2975;

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
  bool vin_uv:1;
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
  bool iout_oc_fault:1;
  bool iout_oc_warning:1;
  bool iout_uc_fault:1;
  bool vin_ov_fault:1;
  bool vin_ov_warning:1;
  bool vin_uv_warning:1;
  bool vin_uv_fault:1;
  bool unit_off_for_insufficient_vin:1;
  bool vout_discharge_fault:1;
  bool fault1_in:1;
  bool fault0_in:1;
  bool watchdog_fault:1;
  bool temp_ot_fault:1;
  bool temp_ot_warning:1;
  bool temp_ut_warning:1;
  bool temp_ut_fault:1;
  bool invalid_command:1;
  bool invalid_data:1;
  bool pec_failed:1;
  bool memory_fault:1;
  bool processor_fault:1;
  bool other_communication_fault:1;
} ltc2975_fault_status;

uint8_t ltc2975_init(LTC2975 *ltc, I2CDriver *i2c, i2caddr_t address,
                     float voltage1, const char *name1, float sense1_mohms,
                     float voltage2, const char *name2, float sense2_mohms,
                     float voltage3, const char *name3, float sense3_mohms,
                     float voltage4, const char *name4, float sense4_mohms);
bool ltc2975_is_alerting(LTC2975 *ltc);
uint8_t ltc2975_clear_faults(LTC2975 *ltc);
ltc2975_fault_status ltc2975_get_fault_status(LTC2975 *ltc, uint8_t page);
bool ltc2975_is_faulting(ltc2975_fault_status *fault_status);
uint8_t ltc2975_check_comms(LTC2975 *ltc);
uint8_t ltc2975_poll(LTC2975 *ltc);
uint8_t ltc2975_turn_on(LTC2975 *ltc, uint8_t channel);
uint8_t ltc2975_turn_off(LTC2975 *ltc, uint8_t channel);

#endif /* LTC2975_H_ */
