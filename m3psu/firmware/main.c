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

static const I2CConfig i2cfg = {OPMODE_SMBUS_HOST, 100000, STD_DUTY_CYCLE};
static const CANConfig cancfg = {CAN_MCR_ABOM | CAN_MCR_AWUM | CAN_MCR_TXFP,
                                 CAN_BTR_SJW(0) | CAN_BTR_TS2(1)
                                     | CAN_BTR_TS1(8) | CAN_BTR_BRP(6)};

static THD_WORKING_AREA(waPowerManager, 1024);
//static THD_WORKING_AREA(waChargeController, 1024);
//static THD_WORKING_AREA(waChargerWatchdog, 256);
static THD_WORKING_AREA(waPowerAlert, 512);

int main(void) {
  
  halInit();
  chSysInit();
  
  palClearLine(LINE_EN_INT_PWR); // Enable internal power

  /*
    // PB8 is CAN1_RC (alternate function 9)
    palSetPadMode(GPIOB, 8, PAL_MODE_ALTERNATE(9));
    // PB9 is CAN1_TX (alternate function 9)
    palSetPadMode(GPIOB, 9, PAL_MODE_ALTERNATE(9));

    // Set PA5 and PA6 to analog inputs
    palSetGroupMode(GPIOA, PAL_PORT_BIT(5) | PAL_PORT_BIT(6), 0, PAL_MODE_INPUT_ANALOG);
  */
  
  i2cStart(&I2C_DRIVER, &i2cfg);
  //canStart(&CAN_DRIVER, &cancfg);
  
  PowerManager_init();
  //ChargeController_init();

  chThdCreateStatic(waPowerAlert, sizeof(waPowerAlert), NORMALPRIO,
                    powermanager_alert, NULL);
  chThdCreateStatic(waPowerManager, sizeof(waPowerManager), NORMALPRIO,
                    powermanager_thread, NULL);

  //chThdCreateStatic(waChargeController, sizeof(waChargeController), NORMALPRIO,
  //                  chargecontroller_thread, NULL);
  //chThdCreateStatic(waChargerWatchdog, sizeof(waChargerWatchdog), NORMALPRIO,
  //                  charger_watchdog_thread, NULL);

  // TESTS

  // Just power on channel 0
  PowerManager_switch_on(0);

  while (true);
}
