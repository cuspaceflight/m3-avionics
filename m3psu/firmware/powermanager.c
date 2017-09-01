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

#define NUM_LTC2975s 3
LTC2975 LTC2975s[NUM_LTC2975s];

LTC4151 currentMonitor;

void PowerManager_init(){

  // TODO: register IRQ for ~ALERT line
  int i = 0;
  for(i=0; i<6; i++){
    m3status_set_init(M3STATUS_COMPONENT_DCDC1 + i);
  }
  m3status_set_init(M3STATUS_COMPONENT_PYRO_MON);

  // Wait for LTC2975s to boot up
  chThdSleepMilliseconds(500);

  // Setup all LTC2975s
  // DCDC-5V
  if(ltc2975_init(&LTC2975s[0], &I2C_DRIVER, 0x5C, 5.0f, "5V CAN", 10,
      5.0f, "5V Cameras", 10, 5.0f, "5V IMU", 10, 5.0f, "5V Radio", 10) == ERR_OK){
    m3status_set_ok(M3STATUS_COMPONENT_DCDC1);
  }else{
    m3status_set_error(M3STATUS_COMPONENT_DCDC1, M3STATUS_DCDC_ERROR_INIT);
  }

  // DCDC-3V
  if(ltc2975_init(&LTC2975s[1], &I2C_DRIVER, 0x5D, 3.3f, "3V3 IMU", 10,
      3.3f, "3V3 Radio", 10, 3.3f, "3V3 FC", 10, 3.3f, "3V3 Pyro", 10) == ERR_OK){
    m3status_set_ok(M3STATUS_COMPONENT_DCDC2);
  }else{
    m3status_set_error(M3STATUS_COMPONENT_DCDC2, M3STATUS_DCDC_ERROR_INIT);
  }
  if(ltc2975_init(&LTC2975s[2], &I2C_DRIVER, 0x5E, 3.3f, "3V3 DL", 10,
      3.3f, "3V3 Base", 10, 0.0f, "", 10, 0.0f, "", 10) == ERR_OK){
    m3status_set_ok(M3STATUS_COMPONENT_DCDC3);
  }else{
    m3status_set_error(M3STATUS_COMPONENT_DCDC3, M3STATUS_DCDC_ERROR_INIT);
  }

  // Setup LTC4151
  if(ltc4151_init(&currentMonitor, &I2C_DRIVER, 0x6F, 0.01f) == ERR_OK){
    m3status_set_ok(M3STATUS_COMPONENT_PYRO_MON);
  }else{
    m3status_set_error(M3STATUS_COMPONENT_PYRO_MON, M3STATUS_PYRO_MON_ERROR_INIT);
  }

  ltc4151_poll(&currentMonitor);

}

void PowerManager_shutdown(){
  int i;
  // Shutdown all channels
  for(i = 0; i < 12; i++){
    PowerManager_switch_off(i);
  }
  PowerManager_disable_pyros();
  PowerManager_disable_system_power();
}

void PowerManager_enable_system_power(){
  palSetLine(LINE_EN_POWER);
}
void PowerManager_disable_system_power(){
  palClearLine(LINE_EN_POWER);
}

void PowerManager_enable_pyros(){
  palSetLine(LINE_EN_PYRO);
}
void PowerManager_disable_pyros(){
  palClearLine(LINE_EN_PYRO);
}

uint8_t PowerManager_switch_on(uint8_t channel){
  if(channel > 4*NUM_LTC2975s) return ERR_INVAL;
  return ltc2975_turn_on(&LTC2975s[channel / 4], channel % 4);
}

uint8_t PowerManager_switch_off(uint8_t channel){
  if(channel > 4*NUM_LTC2975s) return ERR_INVAL;
  return ltc2975_turn_off(&LTC2975s[channel / 4], channel % 4);
}

static thread_t *alert_tp = NULL;
static eventmask_t alert_event = 1;

// TODO test this function
THD_FUNCTION(powermanager_alert, arg){
  (void)arg;
  alert_tp = chThdGetSelfX();
  chRegSetThreadName("PSU Alert Monitor");

  ltc2975_fault_status fault_status;
  uint8_t idx = 0;

  while(TRUE){
    chEvtWaitAny(alert_event);

    // TODO: check for charger ~INT line causing ALERT

    for(idx=0; idx<NUM_LTC2975s*4; idx++){
      if(ltc2975_is_alerting(&LTC2975s[idx/4])){
        // Read each channel and set its status
        fault_status = ltc2975_get_fault_status(&LTC2975s[idx/4], idx%4);
        if(ltc2975_is_faulting(&fault_status)){
          // TODO: more detailed error codes?
          m3status_set_error(M3STATUS_COMPONENT_DCDC1 + (idx/4), M3STATUS_DCDC_ERROR_CH1_ALERT + (idx%4));
        }
      }
    }
  }
}

CH_IRQ_HANDLER(ltc2975AlertIRQ){
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

    // Generate packets for each pair of channels, visiting each LTC2975 twice
    for(idx = 0; idx < (NUM_LTC2975s*4)/2; idx++){
      if(ltc2975_poll(&LTC2975s[idx/2]) == ERR_OK){
        // Voltage as multiple of 0.03V
        // Current as multiple of 0.003A
        // Power as multiple of 0.02W
        can_data[0] = (uint8_t) ((LTC2975s[idx/2].vout[(idx%2)*2+0] * 100.0f) / 3.0f);
        can_data[1] = (uint8_t) ((LTC2975s[idx/2].iout[(idx%2)*2+0] * 1000.0f) / 3.0f);
        can_data[2] = (uint8_t) ((LTC2975s[idx/2].pout[(idx%2)*2+0] * 100.0f) / 2.0f);
        can_data[3] = 0;
        can_data[4] = (uint8_t) ((LTC2975s[idx/2].vout[(idx%2)*2+1] * 100.0f) / 3.0f);
        can_data[5] = (uint8_t) ((LTC2975s[idx/2].iout[(idx%2)*2+1] * 1000.0f) / 3.0f);
        can_data[6] = (uint8_t) ((LTC2975s[idx/2].pout[(idx%2)*2+1] * 100.0f) / 2.0f);
        can_data[7] = 0;

        uint8_t base_id = CAN_MSG_ID_M3PSU_CHANNEL_STATUS_12 >> 5;
        m3can_send(CAN_ID_M3PSU | (CAN_MSG_ID((base_id + idx))), false, can_data, 8);

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

      m3can_send(CAN_MSG_ID_M3PSU_PYRO_STATUS, false, can_data, 8);

      m3status_set_ok(M3STATUS_COMPONENT_PYRO_MON);
    }else{
      m3status_set_error(M3STATUS_COMPONENT_PYRO_MON, M3STATUS_PYRO_MON_ERROR_COMMS);
    }

    chThdSleepMilliseconds(100);
  }
}
