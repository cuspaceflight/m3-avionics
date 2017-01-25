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

#include "bq40z60.h"

BQ40Z60 charger;

void ChargeController_init(void) {
  m3status_set_init(M3STATUS_COMPONENT_CHARGER);

  if(bq40z60_init(&charger, &I2C_DRIVER, 0x0B) == ERR_OK){
    m3status_set_ok(M3STATUS_COMPONENT_CHARGER);
  }else{
    m3status_set_error(M3STATUS_COMPONENT_CHARGER, M3STATUS_CHARGER_ERROR_INIT);
  }

  // Disable charger
  ChargeController_disable_charger();
}

void ChargeController_enable_charger(void) {
  if(bq40z60_set_charger_enabled(&charger, TRUE)!=ERR_OK){
      m3status_set_error(M3STATUS_COMPONENT_CHARGER, M3STATUS_CHARGER_ERROR_READ);
  }
}
void ChargeController_disable_charger(void) {
  if(bq40z60_set_charger_enabled(&charger, FALSE)!=ERR_OK){
      m3status_set_error(M3STATUS_COMPONENT_CHARGER, M3STATUS_CHARGER_ERROR_READ);
  }
}

void ChargeController_enable_balancing(void){
  bq40z60_set_balancing_enabled(&charger, TRUE);
}

void ChargeController_disable_balancing(void){
  bq40z60_set_balancing_enabled(&charger, FALSE);
}

bool ChargeController_is_charger_enabled(void){
    uint8_t enabled = 0;
    if(bq40z60_is_charger_enabled(&charger, &enabled)!=ERR_OK){
        m3status_set_error(M3STATUS_COMPONENT_CHARGER, M3STATUS_CHARGER_ERROR_READ);
    }
    return enabled != 0;
}

bool ChargeController_is_charger_overcurrent(void) {
  if(!ChargeController_is_charger_enabled()){
      return FALSE;
  }
  //TODO READ CHARGER STATUS?
  return FALSE;
}

bool ChargeController_is_adapter_present(void) {
  if(!ChargeController_is_charger_enabled()){
    return FALSE;
  }
  //TODO READ CHARGER STATUS?
  return FALSE;
}

bool ChargeController_is_charging(void){
  //TODO Just read status, see if CHG bit is set

  // If adapter not present, not charging
  if(!ChargeController_is_adapter_present()){
    return FALSE;
  }
  // If charger disabled, not charging
  if(!ChargeController_is_charger_enabled()){
      return FALSE;
  }
  // If charge current / voltage are 0, not charging
  uint16_t val = 0;
  //TODO READ CHARGE VOLTAGE?
  //max17435_get_charge_voltage(&charger, &val);
  if (val == 0){
    return FALSE;
  }
  //TODO READ CHARGE CURRENT?
  //max17435_get_charge_current(&charger, &val);
  if (val == 0){
    return FALSE;
  }

  return TRUE;
}

THD_FUNCTION(chargecontroller_thread, arg) {
  (void)arg;
  chRegSetThreadName("Charge Monitor");

  msg_t status = 0;
  uint8_t can_data[2];

  while (!chThdShouldTerminateX()) {
    // Measure battery voltages
    //TODO READ CELL VOLTAGES

    //TODO READ RSOC, TIME_TO_EMPTY


    // report voltages in multiples of 0.02v
    can_data[0] = (uint8_t) ((batt1 * 100) / 2);
    can_data[1] = (uint8_t) ((batt2 * 100) / 2);

    can_send(CAN_MSG_ID_M3PSU_BATT_VOLTAGES, false, can_data, sizeof(can_data));

    chThdSleepMilliseconds(1);

    // Poll total system current
    uint16_t ma = 0;
    //TODO READ SYSTEM CURRENT
    if(status == ERR_OK){
      m3status_set_ok(M3STATUS_COMPONENT_CHARGER);
      can_data[0] = ma & 0xff;
      can_data[1] = (ma >> 8) & 0xff;
    }else{
      m3status_set_error(M3STATUS_COMPONENT_CHARGER, M3STATUS_CHARGER_ERROR_READ);
      can_data[0] = -1;
      can_data[1] = -1;
    }
    can_data[2] = ((ChargeController_is_adapter_present() ? 1 : 0) << 2) |
                  ((ChargeController_is_charger_overcurrent() ? 1 : 0) << 1) |
                  (ChargeController_is_charger_enabled() ? 1 : 0);
    can_send(CAN_MSG_ID_M3PSU_CHARGER_STATUS, false, can_data, 3);

    chThdSleepMilliseconds(100);
  }
}
