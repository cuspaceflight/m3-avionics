#include "ch.h"
#include "hal.h"
#include "err_handler.h"

void err(uint8_t arg) {
	/* Set Error LED and Halt for now */
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
 *      0x02 = ltc2983_setup - Init Interupt 
 *	       triggered but cmd status does  
 *	       not have DONE flag set [NON FATAL]
 *
 *
 *
 *
 *
 *
 *
 */

/* TODO: Implement an IF statement that causes
 *	 the function to act based on the error
 *	 code and to log the event occurence
 */
