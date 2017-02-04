/*
 * Charge controller and balancer
 * Cambridge University Spaceflight
 */

#include "hal.h"
#include "config.h"
#include "error.h"
#include "m3can.h"
#include "m3status.h"
#include "chargecontroller.h"

#include "max17435.h"
#include "pcal9538a.h"

MAX17435 charger;
PCAL9538A portEx;

#define CHARGER_ENABLE      (3)
#define VEXT_NOT_OK         (2)
#define CHARGER_NOT_OL      (1)
#define VEXT_ENABLE         (0)

static bool shouldCharge = FALSE;
static bool shouldBalance = FALSE;

static const ADCConversionGroup adcgrpcfg = {
  FALSE, // circular mode disabled
  2, // 2 channels
  NULL, // no end callback
  NULL, // no error callback
  0, // no ACD_CR1 settings
  ADC_CR2_SWSTART, // started by software
  0, // no sample times for channels 10-18
  ADC_SMPR2_SMP_AN5(ADC_SAMPLE_144) | ADC_SMPR2_SMP_AN6(ADC_SAMPLE_144), // 144-cycle samples for channels 5,6
  ADC_SQR1_NUM_CH(2), // 2 channels in sequence
  0, // no channels in sequence positions 7-12
  ADC_SQR3_SQ2_N(ADC_CHANNEL_IN6) | ADC_SQR3_SQ1_N(ADC_CHANNEL_IN5) // sample IN5 then IN6
};

void ChargeController_init(void) {

  m3status_set_init(M3STATUS_COMPONENT_CHARGER);
  m3status_set_init(M3STATUS_COMPONENT_ADC);

  // Setup charger to 8.4V at 0.75A, 10mohm sense resistor
  if(max17435_init(&charger, &I2C_DRIVER, 0x09, 8400, 750, 10) == ERR_OK){
    m3status_set_ok(M3STATUS_COMPONENT_CHARGER);
  }else{
    m3status_set_error(M3STATUS_COMPONENT_CHARGER, M3STATUS_CHARGER_ERROR_INIT);
  }

  // Setup port-expander
  // P0 is enable_vext, P1 is charger_not_overcurrent, P2 is vext_not_ok, P3 is
  //  enable_charger
  // TODO need to latch inputs?
  pcal9538a_init(&portEx, &I2C_DRIVER, 0x70);
  pcal9538a_set_pull_up_down(&portEx, 0xFF); // all pull-ups
  pcal9538a_set_output_type(&portEx, 0x00); // all push-pull
  pcal9538a_set_pull_enabled(&portEx, 0xF0); // enable 7-4 pull-ups
  pcal9538a_set_output_dir(&portEx, 0xF6); // 7-4,2-1 in, 3,0 out
  //pcal9538a_set_interrupt_mask(&portEx, 0xF9); // interrupt on 2-1

  pcal9538a_write_output_bit(&portEx, CHARGER_ENABLE, 0);
  pcal9538a_write_output_bit(&portEx, VEXT_ENABLE, 0);
}

uint8_t ChargeController_enable_charger(void) {

  if(pcal9538a_write_output_bit(&portEx, VEXT_ENABLE, 1) != MSG_OK){
    return ERR_COMMS;
  }

  if(pcal9538a_write_output_bit(&portEx, CHARGER_ENABLE, 1) != MSG_OK){
    return ERR_COMMS;
  }

  if(max17435_set_charge_voltage(&charger, charger.config.charge_voltage_mv) != MSG_OK){
    return ERR_COMMS;
  }

  if(max17435_set_charge_current(&charger, charger.config.charge_current_ma) != MSG_OK){
    return ERR_COMMS;
  }

  shouldCharge = TRUE;

  uint16_t ma;
  max17435_get_current(&charger, &ma);

  return ERR_OK;
}
uint8_t ChargeController_disable_charger(void) {
  shouldCharge = FALSE;

  max17435_set_charge_voltage(&charger, 0);
  max17435_set_charge_current(&charger, 0);

  pcal9538a_write_output_bit(&portEx, CHARGER_ENABLE, 0);
  pcal9538a_write_output_bit(&portEx, VEXT_ENABLE, 0);

  return ERR_OK;
}

void ChargeController_enable_balancing(void){
  shouldBalance = TRUE;
}

void ChargeController_disable_balancing(void){
  shouldBalance = FALSE;
}

static bool is_charger_enabled(void){
  uint8_t enabled;
  pcal9538a_read_output_bit(&portEx, CHARGER_ENABLE, &enabled);
  if (enabled == 0){
    return FALSE;
  }
  return TRUE;
}

bool ChargeController_is_charger_overcurrent(void) {
  if(!is_charger_enabled()){
    return FALSE;
  }
  uint8_t result;
  if (pcal9538a_read_inputs(&portEx, &result) != 0) {
    return TRUE; // TODO: what to do if we fail to read status??
  }
  return (result & (1<<CHARGER_NOT_OL)) == 0;
}

bool ChargeController_is_adapter_present(void) {
  if(!is_charger_enabled()){
    return FALSE;
  }
  uint8_t result;
  if (pcal9538a_read_inputs(&portEx, &result) != 0) {
    return FALSE; // TODO: what to do if we fail to read status??
  }
  return (result & (1<<VEXT_NOT_OK)) == 0;
}

bool ChargeController_is_charging(void){
  // If adapter not present, not charging
  if(!ChargeController_is_adapter_present()){
    return FALSE;
  }
  // If charger disabled, not charging
  uint8_t enabled;
  pcal9538a_read_output_bit(&portEx, CHARGER_ENABLE, &enabled);
  if (enabled == FALSE){
    return FALSE;
  }
  // If charge current / voltage are 0, not charging
  uint16_t val;
  max17435_get_charge_voltage(&charger, &val);
  if (val == 0){
    return FALSE;
  }
  max17435_get_charge_current(&charger, &val);
  if (val == 0){
    return FALSE;
  }

  return TRUE;
}

THD_FUNCTION(chargecontroller_thread, arg) {
  (void)arg;
  chRegSetThreadName("Charge Monitor");

  msg_t status;
  adcsample_t samplebuf[2];
  uint8_t can_data[3];

  while (!chThdShouldTerminateX()) {
    // Measure battery voltages
    adcStart(&ADC_DRIVER, NULL);
    adcAcquireBus(&ADC_DRIVER);
    status = adcConvert(&ADC_DRIVER, &adcgrpcfg, samplebuf, 1);
    adcReleaseBus(&ADC_DRIVER);
    adcStop(&ADC_DRIVER);

    if(status == MSG_OK){
      // BATT2 is samplebuf[0], BATT1 is samplebuf[1]
      float batt2 = ((float)samplebuf[0] * 3.3f) / 4096.0f;
      float batt1 = ((float)samplebuf[1] * 3.3f) / 4096.0f;

      batt2 *= 4; // BATT2 has a 1/4 divider
      batt1 *= 2; // BATT1 has a 1/2 divider

      batt2 -= batt1; // get batt2 as a cell voltage

      m3status_set_ok(M3STATUS_COMPONENT_ADC);

      // Balance the batteries
      if(shouldBalance){
        float diff = batt2 - batt1;
        if(diff > 0.05f){ // Batt2 is higher, so bleed it
          palClearLine(LINE_BLEED_BATT_1);
          palSetLine(LINE_BLEED_BATT_2);
        }else if(diff < -0.05f){
          palSetLine(LINE_BLEED_BATT_1);
          palClearLine(LINE_BLEED_BATT_2);
        }else{
          palClearLine(LINE_BLEED_BATT_1);
          palClearLine(LINE_BLEED_BATT_2);
        }
      }else{
        palClearLine(LINE_BLEED_BATT_1);
        palClearLine(LINE_BLEED_BATT_2);
      }

      // TODO: calc battery level etc?


      // report voltages in multiples of 0.02v
      can_data[0] = (uint8_t) ((batt1 * 100) / 2);
      can_data[1] = (uint8_t) ((batt2 * 100) / 2);
      can_data[2] = ((shouldBalance ? 1 : 0) << 2) |
                    (palReadLine(LINE_BLEED_BATT_1) << 1) |
                    palReadLine(LINE_BLEED_BATT_2);

      m3can_send(CAN_MSG_ID_M3PSU_BATT_VOLTAGES, false, can_data, 3);
    }else{
      m3status_set_error(M3STATUS_COMPONENT_ADC, M3STATUS_ADC_ERROR_READ);
    }

    chThdSleepMilliseconds(1);

    // Poll total system current
    uint16_t ma;
    status = max17435_get_current(&charger, &ma);
    if(status == ERR_OK){
      m3status_set_ok(M3STATUS_COMPONENT_CHARGER);
      can_data[0] = ma & 0xff;
      can_data[1] = (ma >> 8) & 0xff;
    }else{
      if(is_charger_enabled()){
        m3status_set_error(M3STATUS_COMPONENT_CHARGER, M3STATUS_CHARGER_ERROR_READ);
      }
      can_data[0] = -1;
      can_data[1] = -1;
    }
    can_data[2] = ((ChargeController_is_adapter_present() ? 1 : 0) << 2) |
                  ((ChargeController_is_charger_overcurrent() ? 1 : 0) << 1) |
                  (shouldCharge ? 1 : 0);
    m3can_send(CAN_MSG_ID_M3PSU_CHARGER_STATUS, false, can_data, 3);

    chThdSleepMilliseconds(100);
  }
}

THD_FUNCTION(charger_watchdog_thread, arg) {
  (void)arg;
  chRegSetThreadName("Charger watchdog");
  while (!chThdShouldTerminateX()) {
    if (shouldCharge) {
      // Send charge voltage / current every 30 seconds
      // The MAX17435 will stop charging if we don't send for 140 seconds
      max17435_set_charge_voltage(&charger, charger.config.charge_voltage_mv);
      max17435_set_charge_current(&charger, charger.config.charge_current_ma);
    }
    chThdSleepSeconds(30);
  }
}
