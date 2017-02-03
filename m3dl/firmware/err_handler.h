#ifndef ERR_HANDLER_H
#define ERR_HANDLER_H

#define M3DL_COMPONENT_LTC2983  1
#define M3DL_COMPONENT_SD_CARD  2
#define M3DL_COMPONENT_PRESSURE 3


#define M3DL_ERROR_LTC2983_OVERFLOW         0x01
#define M3DL_ERROR_LTC2983_SETUP            0x02
#define M3DL_ERROR_SD_CARD_CONNECTION       0x03
#define M3DL_ERROR_SD_CARD_MOUNTING         0x04
#define M3DL_ERROR_SD_CARD_FILE_OPEN        0x05
#define M3DL_ERROR_SD_CARD_INC_FILE_OPEN    0x06
#define M3DL_ERROR_SD_CARD_WRITE            0x07
#define M3DL_ERROR_LOGGING_WRITE            0x08
#define M3DL_ERROR_SD_CARD_FULL             0x09

#define M3DL_ERROR_TEMP1_INVALID            0x10
#define M3DL_ERROR_TEMP2_INVALID            0x11
#define M3DL_ERROR_TEMP3_INVALID            0x12
#define M3DL_ERROR_TEMP4_INVALID            0x13
#define M3DL_ERROR_TEMP5_INVALID            0x14
#define M3DL_ERROR_TEMP6_INVALID            0x15
#define M3DL_ERROR_TEMP7_INVALID            0x16
#define M3DL_ERROR_TEMP8_INVALID            0x17
#define M3DL_ERROR_TEMP9_INVALID            0x18

#define M3DL_ERROR_CRC_FAILED               0x19
#define M3DL_ERROR_PRESSURE_TIMEOUT         0x20

/* Internal Error Handler */
void err(uint8_t arg);

#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * TABLE OF ERROR CODES: * * * * * * *	
 * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *	0x01 = ltc2983_write_reg - SPI TX
 *         buffer overflow [LTC2893.c]
 *
 *  0x02 = ltc2983_setup - Attempted to setup 
 *	       config registers but cmd status does  
 *	       not have DONE flag set [LTC2893.c]
 *
 *	0x03 = microsd_card_init - SD card 
 *	       connection failed [microsd.c]
 *
 *	0x04 = microsd_card_init - SD card
 *	       mounting failed [microsd.c]
 *
 *	0x05 = microsd_open_file - SD card
 *	       failed to open file [microsd.c]
 *
 *	0x06 = microsd_open_file_inc - SD card
 *	       failed to open inc file [microsd.c]
 *
 *	0x07 = microsd_write - SD card write
 *	       failed [microsd.c]
 *
 *  0x08 = microsd_write - SD card write 
 *	       failed [logging.c]
 *
 *  0x09 = microsd_write - SD card full [microsd.c]
 *
 *  0x10
 *   to  = log_temp - Invalid temperature
 *  0x18   data [LTC2983.c]
 *
 *  0x19 = process_buffer - CRC failue [pressure.c]
 *
 *  0x20 = process_buffer - UART timeout [pressure.c]
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * */
