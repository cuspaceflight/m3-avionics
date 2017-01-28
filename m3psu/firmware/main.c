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
#include "smbus.h"
#include "m3can.h"

static THD_WORKING_AREA(waPowerManager, 1024);
static THD_WORKING_AREA(waChargeController, 1024);
//static THD_WORKING_AREA(waPowerAlert, 512);

void enable_system_power(void){
  palClearLine(LINE_EN_POWER);
}
void disable_system_power(void){
  palSetLine(LINE_EN_POWER);
}

void enable_pyros(void){
  palSetLine(LINE_EN_PYRO);
}
void disable_pyros(void){
  palClearLine(LINE_EN_PYRO);
}

void can_recv(uint16_t msg_id, bool rtr, uint8_t *data, uint8_t datalen){
  (void)rtr;

  if(msg_id == CAN_MSG_ID_M3PSU_TOGGLE_PYROS){
    if(datalen >= 1){
      if(data[0] == 0){
        disable_pyros();
      }else if(data[0] == 1){
        enable_pyros();
      }
    }
  }else if(msg_id == CAN_MSG_ID_M3PSU_TOGGLE_CHANNEL){
    if(datalen >= 2){
      if(data[0] == 1){
        PowerManager_switch_on(data[1]);
      }else if(data[0] == 0){
        PowerManager_switch_off(data[1]);
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
  }
}

static THD_WORKING_AREA(waPowerCheck, 512);
THD_FUNCTION(power_check, arg){
  (void)arg;
  chRegSetThreadName("Power Switch Checker");

  while(TRUE){
    bool switch_open = palReadLine(LINE_PWR);
    if(switch_open){
      int i;
      // Shutdown all channels and go to sleep
      for(i = 0; i < 12; i++){
          PowerManager_switch_off(i);
      }
      //disable_external_power();
      disable_pyros();
      disable_system_power();
      //TODO enter deep sleep
    }
    chThdSleepMilliseconds(1000);
  }
}

int main(void) {

  halInit();
  chSysInit();

  enable_pyros();
  enable_system_power();

  smbus_init();
  can_init(CAN_ID_M3PSU);

  PowerManager_init();
  ChargeController_init();

  ChargeController_enable_charger();

  chThdCreateStatic(waPowerCheck, sizeof(waPowerCheck), NORMALPRIO,
                    power_check, NULL);
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
