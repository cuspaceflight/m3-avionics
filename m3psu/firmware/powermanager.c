/*
 * Power Manager
 * Cambridge University Spaceflight
 */

#include "ch.h"

#include "config.h"
#include "error.h"
#include "m3can.h"
#include "m3status.h"
#include "powermanager.h"

#define NUM_LTC3887s    6
LTC3887 LTC3887s[NUM_LTC3887s];

LTC4151 currentMonitor;

void PowerManager_init(){

  // TODO: register IRQ for ~ALERT line
  int i = 0;
  for(i=0; i<6; i++){
    m3status_set_init(M3STATUS_COMPONENT_DCDC1 + i);
  }
  m3status_set_init(M3STATUS_COMPONENT_PYRO_MON);

  // Wait for LTC3887s to boot up
  chThdSleepMilliseconds(500);

  // Setup all LTC3887s
  // Board 1
  if(ltc3887_init(&LTC3887s[0], &I2C_DRIVER, 0x44, "5V IMU", 5.0f, 50, "5V AUX 2", 5.0f, 50) == ERR_OK){
    m3status_set_ok(M3STATUS_COMPONENT_DCDC1);
  }else{
    m3status_set_error(M3STATUS_COMPONENT_DCDC1, M3STATUS_DCDC_ERROR_INIT);
  }
  if(ltc3887_init(&LTC3887s[1], &I2C_DRIVER, 0x45, "3V3 FC", 3.3f, 50, "3V3 IMU", 3.3f, 50) == ERR_OK){
    m3status_set_ok(M3STATUS_COMPONENT_DCDC2);
  }else{
    m3status_set_error(M3STATUS_COMPONENT_DCDC2, M3STATUS_DCDC_ERROR_INIT);
  }
  // Board 2
  if(ltc3887_init(&LTC3887s[2], &I2C_DRIVER, 0x42, "5V Radio", 5.0f, 50, "5V AUX 1", 5.0f, 50) == ERR_OK){
    m3status_set_ok(M3STATUS_COMPONENT_DCDC3);
  }else{
    m3status_set_error(M3STATUS_COMPONENT_DCDC3, M3STATUS_DCDC_ERROR_INIT);
  }
  if(ltc3887_init(&LTC3887s[3], &I2C_DRIVER, 0x43, "3V3 Pyro", 3.3f, 50, "3V3 Radio", 3.3f, 50) == ERR_OK){
    m3status_set_ok(M3STATUS_COMPONENT_DCDC4);
  }else{
    m3status_set_error(M3STATUS_COMPONENT_DCDC4, M3STATUS_DCDC_ERROR_INIT);
  }
  // Board 3
  if(ltc3887_init(&LTC3887s[4], &I2C_DRIVER, 0x46, "5V Cameras", 5.0f, 50, "3V3 AUX 1", 3.3f, 50) == ERR_OK){
    m3status_set_ok(M3STATUS_COMPONENT_DCDC5);
  }else{
    m3status_set_error(M3STATUS_COMPONENT_DCDC5, M3STATUS_DCDC_ERROR_INIT);
  }
  if(ltc3887_init(&LTC3887s[5], &I2C_DRIVER, 0x47, "3V3 DL", 3.3f, 50, "5V CAN", 5.0f, 50) == ERR_OK){
    m3status_set_ok(M3STATUS_COMPONENT_DCDC6);
  }else{
    m3status_set_error(M3STATUS_COMPONENT_DCDC6, M3STATUS_DCDC_ERROR_INIT);
  }

  PowerManager_switch_on(0); // Start the IMU
  PowerManager_switch_on(3);
  PowerManager_switch_on(2); // Start the Flight Computer
  PowerManager_switch_on(4); // Start the Radio
  PowerManager_switch_on(7);
  PowerManager_switch_on(6); // Start the Pyro
  PowerManager_switch_on(10); // Start the DL including base board for USB
  PowerManager_switch_on(11); // Start the CAN transceivers

  // Setup LTC4151
  if(ltc4151_init(&currentMonitor, &I2C_DRIVER, 0x6F, 0.01f) == ERR_OK){
    m3status_set_ok(M3STATUS_COMPONENT_PYRO_MON);
  }else{
    m3status_set_error(M3STATUS_COMPONENT_PYRO_MON, M3STATUS_PYRO_MON_ERROR_INIT);
  }

  ltc4151_poll(&currentMonitor);

}

uint8_t PowerManager_switch_on(uint8_t channel){
  if(channel > 2*NUM_LTC3887s) return ERR_INVAL;
  return ltc3887_turn_on(&LTC3887s[channel / 2], channel % 2);
}

uint8_t PowerManager_switch_off(uint8_t channel){
  if(channel > 2*NUM_LTC3887s) return ERR_INVAL;
  return ltc3887_turn_off(&LTC3887s[channel / 2], channel % 2);
}

static thread_t *alert_tp = NULL;
static eventmask_t alert_event = 1;

THD_FUNCTION(powermanager_alert, arg){
  (void)arg;
  alert_tp = chThdGetSelfX();
  chRegSetThreadName("PSU Alert Monitor");

  ltc3887_fault_status fault_status;
  uint8_t idx = 0;

  while(TRUE){
    chEvtWaitAny(alert_event);

    // TODO: check for charger ~INT line causing ALERT

    for(idx=0; idx<NUM_LTC3887s; idx++){
      if(ltc3887_is_alerting(&LTC3887s[idx])){
        // Channel 0 faulting?
        fault_status = ltc3887_get_fault_status(&LTC3887s[idx], 0);
        if(ltc3887_is_faulting(&fault_status)){
          // TODO: more detailed error codes?
          m3status_set_error(M3STATUS_COMPONENT_DCDC1 + idx, M3STATUS_DCDC_ERROR_CH1_ALERT);
        }
        // Channel 1 faulting?
        fault_status = ltc3887_get_fault_status(&LTC3887s[idx], 1);
        if(ltc3887_is_faulting(&fault_status)){
          // TODO: more detailed error codes?
          m3status_set_error(M3STATUS_COMPONENT_DCDC1 + idx, M3STATUS_DCDC_ERROR_CH2_ALERT);
        }
      }
    }
  }
}

CH_IRQ_HANDLER(ltc3887AlertIRQ){
  CH_IRQ_PROLOGUE();

  // Signal the ALERT thread that the signal has been triggered
  chSysLockFromISR();
  chEvtSignalI(alert_tp, alert_event);
  chSysUnlockFromISR();

  CH_IRQ_EPILOGUE();
}

THD_FUNCTION(powermanager_thread, arg){
  (void)arg;
  uint8_t can_data[8];

  chRegSetThreadName("PSU Poller");

  while(!chThdShouldTerminateX()){
    uint8_t idx;
    uint16_t voltage, current, power;

    for(idx = 0; idx < NUM_LTC3887s; idx++){
      if(ltc3887_poll(&LTC3887s[idx]) == ERR_OK){
        // Voltage as multiple of 0.03V
        // Current as multiple of 0.003A
        // Power as multiple of 0.02W
        can_data[0] = (uint8_t) ((LTC3887s[idx].vout_1 * 100.0f) / 3.0f);;
        can_data[1] = (uint8_t) ((LTC3887s[idx].iout_1 * 1000.0f) / 3.0f);;
        can_data[2] = (uint8_t) ((LTC3887s[idx].pout_1 * 100.0f) / 2.0f);;
        can_data[3] = 0;
        can_data[4] = (uint8_t) ((LTC3887s[idx].vout_2 * 100.0f) / 3.0f);;
        can_data[5] = (uint8_t) ((LTC3887s[idx].iout_2 * 1000.0f) / 3.0f);;
        can_data[6] = (uint8_t) ((LTC3887s[idx].pout_2 * 100.0f) / 2.0f);;
        can_data[7] = 0;

        uint8_t base_id = CAN_MSG_ID_M3PSU_CHANNEL_STATUS_12 >> 5;
        can_send(CAN_ID_M3PSU | (CAN_MSG_ID((base_id + idx))), false, can_data, 8);

        m3status_set_ok(M3STATUS_COMPONENT_DCDC1 + idx);

      }else{
        m3status_set_error(M3STATUS_COMPONENT_DCDC1 + idx, M3STATUS_DCDC_ERROR_COMMS);
      }
      chThdSleepMilliseconds(1);
    }
    if(ltc4151_poll(&currentMonitor) == ERR_OK){
      // Voltage as multiple of 0.001V
      voltage = (uint16_t) (currentMonitor.voltage_v * 1000.0f);
      // Current as multiple of 1mA
      current = (uint16_t) (currentMonitor.current_ma);
      // Power as multiple of 1mW
      power = (uint16_t) (currentMonitor.power_mw);

      can_data[0] = voltage & 0xff;
      can_data[1] = (voltage >> 8) & 0xff;
      can_data[2] = current & 0xff;
      can_data[3] = (current >> 8) & 0xff;
      can_data[4] = power & 0xff;
      can_data[5] = (power >> 8) & 0xff;
      can_data[6] = palReadLine(LINE_EN_PYRO);
      can_data[7] = 0;

      can_send(CAN_MSG_ID_M3PSU_PYRO_STATUS, false, can_data, 8);

      m3status_set_ok(M3STATUS_COMPONENT_PYRO_MON);
    }else{
      m3status_set_error(M3STATUS_COMPONENT_PYRO_MON, M3STATUS_PYRO_MON_ERROR_COMMS);
    }

    chThdSleepMilliseconds(500);
  }
}
