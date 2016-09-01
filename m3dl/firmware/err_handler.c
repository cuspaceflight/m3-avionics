#include "ch.h"
#include "hal.h"
#include "err_handler.h"

void err(uint8_t arg) {
	/* Flash Error LED */
	(void)arg;
	palSetPad(GPIOC, GPIOC_LED2_RED);
	chThdSleepMilliseconds(100);
	palClearPad(GPIOC, GPIOC_LED2_RED);
}

/* * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * TABLE OF ERROR CODES: * * * * * *	
 * * * * * * * * * * * * * * * * * * * * * * *
 *
 *	0x01 = ltc2983_write_reg - SPI TX
 *             buffer overflow [LTC2893.c]
 *
 *  0x02 = ltc2983_setup - Init interupt 
 *	       triggered but cmd status does  
 *	       not have DONE flag set [LTC2893.c]
 *
 *	0x03 = microsd_card_init - SD card 
 *	       connection failed [microsd.c]
 *
 *	0x04 = microsd_card_init - SD card
 *	       mounting failed [microsd.c]
 *
 *	0x05 = microsd_open_file - SD card
 *	       init failed [microsd.c]
 *
 *	0x06 = microsd_open_file_inc - SD card
 *	       incremental init failed [microsd.c]
 *
 *	0x07 = microsd_write - SD card write
 *	       failed [microsd.c]

	0x08 = microsd_write - SD card write 
	       failed [logging.c]
 *
 *
 */

/* TODO: Implement an IF statement that causes
 *	 the function to act based on the error
 *	 code and to log the event occurence
 */
