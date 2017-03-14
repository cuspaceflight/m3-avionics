/*
 * Power Manager
 * Cambridge University Spaceflight
 */

#ifndef POWERMANAGER_H_
#define POWERMANAGER_H_

#include "ch.h"

#include "ltc3887.h"
#include "ltc4151.h"

void PowerManager_init(void);

void PowerManager_shutdown(void);

void PowerManager_enable_system_power(void);
void PowerManager_disable_system_power(void);

void PowerManager_enable_pyros(void);
void PowerManager_disable_pyros(void);

uint8_t PowerManager_switch_on(uint8_t channel);
uint8_t PowerManager_switch_off(uint8_t channel);

CH_IRQ_HANDLER(ltc3887AlertIRQ);

THD_FUNCTION(powermanager_alert, arg);
THD_FUNCTION(powermanager_thread, arg);

#endif /* POWERMANAGER_H_ */
