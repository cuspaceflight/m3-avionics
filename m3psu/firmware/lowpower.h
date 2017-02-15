/*
 * Low Power Mode
 * Cambridge University Spaceflight
 */

#ifndef LOWPOWER_H_
#define LOWPOWER_H_

#include "ch.h"

#include "backupregs.h"

#define LOWPOWER_POWER_SWITCH_INTERVAL 5 // Check power switch every 5 seconds
#define LOWPOWER_SLEEP_TIME 600 // Sleep for 10 mins
#define LOWPOWER_AWAKE_TIME 180 // Awake for 3 mins

void lowpower_init(void);

THD_FUNCTION(lowpower_power_check_thread, arg);

void lowpower_early_wakeup_check(void);
void lowpower_start_awake_timer(void);
void lowpower_setup_sleep(uint16_t seconds);
void lowpower_go_to_sleep(void);

void lowpower_enable(void);
void lowpower_disable(void);

bool lowpower_get_mode_flag(void);
void lowpower_set_mode_flag(bool enabled);

#endif /* LOWPOWER_H_ */
