#include "ch.h"
#include "hal.h"
#include "err_handler.h"
#include "m3status.h"

void err(uint8_t arg) {
	/* Flash Error LED */
	(void)arg;
	palSetPad(GPIOC, GPIOC_LED2_RED);
	chThdSleepMilliseconds(100);
	palClearPad(GPIOC, GPIOC_LED2_RED);
}
