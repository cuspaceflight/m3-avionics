#include "ch.h"
#include "hal.h"
#include "err_handler.h"

void err(uint8_t arg) {
	/* Do nothing with argument for now */
	(void)arg;
	palSetPad(GPIOC, GPIOC_LED2_RED);
	chSysHalt(NULL);
}

/* * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * TABLE OF ERROR CODES: * * * * * *	
 * * * * * * * * * * * * * * * * * * * * * * *
 *
 *	0x01 = ltc2983_write_reg - SPI TX
 *             buffer overflow [NON FATAL]
 *
 *      0x02 = 
 *
 *
 *
 *
 *
 *
 *
 *
 *
 */
