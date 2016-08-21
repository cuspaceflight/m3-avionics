/*
 * Charge controller and balancer
 * Cambridge University Spaceflight
 */

#ifndef CHARGECONTROLLER_H_
#define CHARGECONTROLLER_H_

#include "ch.h"

#include "max17435.h"
#include "pcal9538a.h"

void ChargeController_init(void);

uint8_t ChargeController_enable_charger(void);
uint8_t ChargeController_disable_charger(void);

bool ChargeController_is_charger_overcurrent(void);
bool ChargeController_is_adapter_present(void);

bool ChargeController_is_charging(void);

THD_FUNCTION(charger_watchdog_thread, arg);
THD_FUNCTION(chargecontroller_thread, arg);

#endif /* CHARGECONTROLLER_H_ */
