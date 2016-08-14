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

uint8_t PowerManager_switch_on(uint8_t channel);
uint8_t PowerManager_switch_off(uint8_t channel);

CH_IRQ_HANDLER(ltc3887AlertIRQ);

THD_FUNCTION(powermanager_alert, arg);
THD_FUNCTION(powermanager_thread, arg);

#endif /* POWERMANAGER_H_ */
