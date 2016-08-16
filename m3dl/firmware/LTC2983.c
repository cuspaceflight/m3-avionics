#include "ch.h"
#include "hal.h"
#include "LTC2983.h"
#include <string.h>
#include "err_handler.h"

/* Function Prototypes */
static void ltc2983_write_reg(uint16_t addr, size_t len, uint8_t* data);
static void ltc2983_read_reg(uint16_t addr, size_t len, uint8_t* data);
static void ltc2983_setup(void);

/* Set SPI Config */
static const SPIConfig hs_spicfg = {
  .end_cb = NULL,
  .ssport = GPIOA,
  .sspad = GPIOA_SPI1_CS,
  /* Set Baud Rate to 168/2/64 = 1.31 MHz */
  .cr1 = SPI_CR1_BR_2 | SPI_CR1_BR_0
};


/* TEMP INT Callback */
void temp_ready(EXTDriver *extp, expchannel_t channel) {

    (void)extp;
    (void)channel;

}

/* LTC2983 Thread */
static THD_WORKING_AREA(ltc2983_wa, 256);
static THD_FUNCTION(ltc2983_thd, arg) {
	
    /* Set Thread Name & Start SPI */
    (void)arg;
    chRegSetThreadName("LTC2983");
    spiStart(&SPID1, &hs_spicfg);

    /* 
     * Sleep until LTC2983 power up fires
     * interrupt taking TEMP_INT HIGH  
     */
 
    chThdSleepMilliseconds(200);	

    /* Call ltc2983_setup function */

     ltc2983_setup();

    /* 
     * Enter while loop that continually starts
     * a multi-channel conversion and sleeps until
     * completion is signalled by the LTC2983 
     * taking the TEMP_INT pin high  
     */

    while(TRUE) {
        /* Do Nothing */
	palSetPad(GPIOC, GPIOC_LED2_RED);
        chThdSleepMilliseconds(500);
	palClearPad(GPIOC, GPIOC_LED2_RED);
        chThdSleepMilliseconds(500);
    }
  
}

/* Entry Point */
void ltc2983_init(void) {
    /* Create LTC2983 Thread */
    chThdCreateStatic(ltc2983_wa, sizeof(ltc2983_wa),
                      NORMALPRIO, ltc2983_thd, NULL);
}


/* Write to LTC2983 */
static void ltc2983_write_reg(uint16_t addr, size_t len, uint8_t* data) {
	
	/* Setup TX Buffer */
	uint8_t txbuf[83];

	/* Check Data Length */
	if (len > 80){
	    err(0x01);
	}
        
        else {

	/* Populate TX Buffer */
	txbuf[0] = 0x02;
	txbuf[1] = addr >> 8;
        txbuf[2] = addr & 0xFF;
	memcpy(&txbuf[3], data, len);
	
	/* SPI Send */
	spiSelect(&SPID1);                  
  	spiSend(&SPID1, len + 3, txbuf);        
 	spiUnselect(&SPID1);                
	
	}
}

/* Read from LTC2983 */
static void ltc2983_read_reg(uint16_t addr, size_t len, uint8_t* data) {
	
	/* Setup TX Buffer */
	uint8_t txbuf[3];

        /* Populate TX Buffer */
	txbuf[0] = 0x03;
	txbuf[1] = addr >> 8;
        txbuf[2] = addr & 0xFF;

	/* SPI Send RX Command */
	spiSelect(&SPID1);                  
  	spiSend(&SPID1, 3, txbuf);        
 	
	/* SPI Reccieve */
	spiReceive(&SPID1, len, data);
	spiUnselect(&SPID1);
}

/* Register Setup */
static void ltc2983_setup(void) {

	/*
	 * Read command status register and check
	 * that 0x40 is returned ensuring power up
         * is complete.
	 */

	uint8_t cmd_status_reg;
	
	ltc2983_read_reg(0x000, 1, &cmd_status_reg);

	if (cmd_status_reg != 0x40) {
		err(0x02);
	}

	/* 
	 * Loop over address to populate sensor config
	 * buffer and send to ltc2983 at 0x200
	 */
	

	/* Buffer to Hold Sensor Config Data */
	uint8_t sensor_config[80];

	/* Loop Pointer */
	uint8_t i;
	i = 0;

	/* Populate Thermocouple Specific Data */
	while (i < 76){
	
		if ((i%8) == 0){
			/* Odd Channel Number */
			sensor_config[i] = 0x00;
			sensor_config[i + 1] = 0x00;
			sensor_config[i + 2] = 0x00;
			sensor_config[i + 3] = 0x00;		
		} 
		else {
			/* Even Channel Number */
			sensor_config[i] = 0x15;
			sensor_config[i + 1] = 0x10;
			sensor_config[i + 2] = 0x00;
			sensor_config[i + 3] = 0x00;	
		}

		i = i + 4;
	}

	
	/* Populate Diode Specific Data */
	sensor_config[76] = 0xE4;
	sensor_config[77] = 0x50;
	sensor_config[78] = 0x0C;
	sensor_config[79] = 0x49;	


	/*
  	 * Send over SPI & return to thread
	 */

	ltc2983_write_reg(0x200, 80, sensor_config);

}

