#ifndef ERR_HANDLER_H
#define ERR_HANDLER_H

#define M3DL_COMPONENT_LTC2983 1
#define M3DL_COMPONENT_SD_CARD 2


#define M3DL_ERROR_LTC2983_OVERFLOW 0x01
#define M3DL_ERROR_LTC2983_SETUP 0x02
#define M3DL_ERROR_SD_CARD_CONNECTION 0x03
#define M3DL_ERROR_SD_CARD_MOUNTING 0x04
#define M3DL_ERROR_SD_CARD_INIT 0x05
#define M3DL_ERROR_SD_CARD_FILE_OPEN 0x06
#define M3DL_ERROR_SD_CARD_WRITE 0x07
#define M3DL_ERROR_LOGGING_WRITE 0x08

/* Error Handler */
void err(uint8_t arg);

#endif

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
