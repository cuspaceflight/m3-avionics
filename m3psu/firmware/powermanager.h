/*
 * Power Manager
 * Cambridge University Spaceflight
 */

#ifndef POWERMANAGER_H_
#define POWERMANAGER_H_

#include "ch.h"

#include "ltc2975.h"
#include "ltc4151.h"

#define POWER_CHANNEL_5V_CAN        0
#define POWER_CHANNEL_5V_CAMERAS    1
#define POWER_CHANNEL_5V_IMU        2
#define POWER_CHANNEL_5V_RADIO      3
#define POWER_CHANNEL_3V3_IMU       4
#define POWER_CHANNEL_3V3_RADIO     5
#define POWER_CHANNEL_3V3_FC        6
#define POWER_CHANNEL_3V3_PYRO      7
#define POWER_CHANNEL_3V3_DL        8
#define POWER_CHANNEL_3V3_BASE      9
#define POWER_CHANNEL_3V3_SPARE1    10
#define POWER_CHANNEL_3V3_SPARE2    11

void PowerManager_init(void);

void PowerManager_shutdown(void);

void PowerManager_enable_system_power(void);
void PowerManager_disable_system_power(void);

void PowerManager_enable_pyros(void);
void PowerManager_disable_pyros(void);

uint8_t PowerManager_switch_on(uint8_t channel);
uint8_t PowerManager_switch_off(uint8_t channel);

//CH_IRQ_HANDLER(ltc3887AlertIRQ);

THD_FUNCTION(powermanager_alert, arg);
THD_FUNCTION(powermanager_thread, arg);

#endif /* POWERMANAGER_H_ */
