/*
 ChibiOS - Copyright (C) 2006..2015 Giovanni Di Sirio

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#include <stdlib.h>

#include "ch.h"
#include "hal.h"

#include "config.h"
#include "powermanager.h"
#include "chargecontroller.h"
#include "lowpower.h"
#include "smbus.h"
#include "m3can.h"

static THD_WORKING_AREA(waPowerManager, 1024);
static THD_WORKING_AREA(waChargeController, 1024);
//static THD_WORKING_AREA(waPowerAlert, 512);
static THD_WORKING_AREA(waPowerCheck, 512);
static THD_WORKING_AREA(waAwakeTime, 128);

void m3can_recv(uint16_t msg_id, bool rtr, uint8_t *data, uint8_t datalen){
  (void)rtr;

  if(msg_id == CAN_MSG_ID_M3PSU_TOGGLE_PYROS){
    if(datalen >= 1){
      if(data[0] == 0){
        PowerManager_disable_pyros();
      }else if(data[0] == 1){
        PowerManager_enable_pyros();
      }
    }
  }else if(msg_id == CAN_MSG_ID_M3PSU_TOGGLE_CHANNEL){
    if(datalen >= 2){
      if(data[0] == 1){
        PowerManager_switch_on(data[1]);
      }else if(data[0] == 0){
        if(data[1] == 4 || data[1] == 7 || data[1] == 11){
          // Ignore turning off the radio or CAN!
        }else{
          PowerManager_switch_off(data[1]);
        }
      }
    }
  }else if(msg_id == CAN_MSG_ID_M3PSU_TOGGLE_CHARGER){
    if(datalen >= 1){
      if(data[0] == 1){
        ChargeController_enable_charger();
      }else if(data[0] == 0){
        ChargeController_disable_charger();
      }
    }
  }else if(msg_id == CAN_MSG_ID_M3PSU_TOGGLE_LOWPOWER){
    if(datalen >= 1){
      if(data[0] == 1){
        lowpower_enable();
      }else if(data[0] == 0){
        lowpower_disable();
      }
    }
  }else if(msg_id == CAN_MSG_ID_M3PSU_TOGGLE_BATTLESHORT){
    if(datalen >= 1){
      if(data[0] == 1){
        ChargeController_enable_battleshort();
      }else if(data[0] == 0){
        ChargeController_disable_battleshort();
      }
    }
  }
}

THD_FUNCTION(awake_time_thread, arg){
  (void)arg;
  while(true){
    systime_t time_now = chVTGetSystemTime();
    uint16_t secs_awake = (uint16_t)(time_now / CH_CFG_ST_FREQUENCY);
    uint8_t data[3];
    data[0] = secs_awake & 0xff;
    data[1] = (secs_awake >> 8) & 0xff;

    data[2] = lowpower_get_mode_flag() ? 1 : 0;

    m3can_send(CAN_MSG_ID_M3PSU_AWAKE_TIME, false, data, sizeof(data));

    chThdSleepMilliseconds(500);
  }
}

int main(void) {
  /* Allow debug access during sleep mode */
  DBGMCU->CR |= DBGMCU_CR_DBG_SLEEP;

  halInit();

  lowpower_early_wakeup_check();

  chSysInit();

  lowpower_init();

  PowerManager_enable_system_power();

  smbus_init(&I2C_DRIVER);

  uint16_t listen_to_ids[] = { CAN_ID_M3PSU };
  m3can_init(CAN_ID_M3PSU, listen_to_ids, 1);

  PowerManager_init();
  ChargeController_init();

  if(lowpower_get_mode_flag()){ // We're supposed to be in low-power mode
    PowerManager_switch_on(4); // Start the Radio
    PowerManager_switch_on(7);
    PowerManager_switch_on(11); // Start the CAN transceivers

    lowpower_start_awake_timer();
  }else{
    lowpower_set_mode_flag(0); // Make sure the flag is cleared

    PowerManager_enable_pyros();

    //PowerManager_switch_on(0); // Start the IMU
    PowerManager_switch_on(3);
    PowerManager_switch_on(2); // Start the Flight Computer
    PowerManager_switch_on(4); // Start the Radio
    PowerManager_switch_on(7);
    PowerManager_switch_on(6); // Start the Pyro
    PowerManager_switch_on(10); // Start the DL including base board for USB
    PowerManager_switch_on(11); // Start the CAN transceivers
  }

  ChargeController_enable_charger();

  chThdCreateStatic(waAwakeTime, sizeof(waAwakeTime), NORMALPRIO, awake_time_thread, NULL);

  chThdCreateStatic(waPowerCheck, sizeof(waPowerCheck), NORMALPRIO,
                    lowpower_power_check_thread, NULL);
  //chThdCreateStatic(waPowerAlert, sizeof(waPowerAlert), NORMALPRIO + 3,
  //                  powermanager_alert, NULL);
  chThdCreateStatic(waPowerManager, sizeof(waPowerManager), NORMALPRIO + 4,
                    powermanager_thread, NULL);

  chThdCreateStatic(waChargeController, sizeof(waChargeController), NORMALPRIO + 2,
                    chargecontroller_thread, NULL);

  // All done, go to sleep forever
  chThdSleep(TIME_INFINITE);

  return 0;
}
