#include "ch.h"
#include "hal.h"
#include "err_handler.h"

void err(uint8_t arg) {
	/* Dummy Argument */
	(void)arg;
	palSetPad(GPIOC, GPIOC_LED2_RED);
	chSysHalt(NULL);
}
