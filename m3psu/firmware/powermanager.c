/*
 * Power Manager
 * Cambridge University Spaceflight
 */

#include "ch.h"

#include "config.h"
#include "error.h"
#include "powermanager.h"

#define NUM_LTC3887s    6
static LTC3887 LTC3887s[NUM_LTC3887s];

static LTC4151 currentMonitor;

void PowerManager_init(){

  // TODO: register IRQ for ~ALERT line

  // Setup all LTC3887s
  
  // Board 1
  ltc3887_init(&LTC3887s[0], &I2C_DRIVER, 0x44, "5V IMU", 5.0f, "5V AUX 2", 5.0f);
  //ltc3887_init(&LTC3887s[1], &I2C_DRIVER, 0x45, "3V3 FC", 3.3f, "3V3 IMU", 3.3f);
  
  /*
  ltc3887_init(&LTC3887s[0], &I2C_DRIVER, 0x44, "5V IMU", 5.0f, "5V AUX 2", 5.0f);
  ltc3887_init(&LTC3887s[1], &I2C_DRIVER, 0x45, "3V3 FC", 3.3f, "3V3 IMU", 3.3f);
  // Board 2
  ltc3887_init(&LTC3887s[2], &I2C_DRIVER, 0x42, "5V Radio", 5.0f, "5V AUX 1", 5.0f);
  ltc3887_init(&LTC3887s[3], &I2C_DRIVER, 0x43, "3V3 Pyro", 3.3f, "3V3 Radio", 3.3f);
  // Board 3
  ltc3887_init(&LTC3887s[4], &I2C_DRIVER, 0x46, "5V Cameras", 5.0f, "3V3 AUX 1", 3.3f);
  ltc3887_init(&LTC3887s[5], &I2C_DRIVER, 0x47, "3V3 DL", 3.3f, "5V CAN", 5.0f);
  */
  

  // Setup LTC4151
  //ltc4151_init(&currentMonitor, &I2C_DRIVER, 0x6F, 0.01f);

  //while(true){
  //  ltc4151_poll(&currentMonitor);
  //}

  while(true);

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
          // TODO: act on alert status;
        }
        // Channel 1 faulting?
        fault_status = ltc3887_get_fault_status(&LTC3887s[idx], 1);
        if(ltc3887_is_faulting(&fault_status)){
          // TODO: act on alert status;
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

  chRegSetThreadName("PSU Poller");

  while(!chThdShouldTerminateX()){
    uint8_t idx;
    for(idx = 0; idx < NUM_LTC3887s; idx++){
      ltc3887_poll(&LTC3887s[idx]);
    }
    ltc4151_poll(&currentMonitor);

    chThdSleepMilliseconds(50);
  }
}
