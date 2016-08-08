#include "ch.h"
#include "hal.h"
#include "LTC2983.h"
#include <string.h>
#include "err_handler.h"

/* Function Prototypes */
static void ltc2983_write_reg(uint16_t addr, uint8_t len, uint8_t* data);
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


/* LTC2983 Thread */
static THD_WORKING_AREA(ltc2983_wa, 256);
static THD_FUNCTION(ltc2983_thd, arg) {
	
   /* Set Thread Name & Start SPI */
   (void)arg;
   chRegSetThreadName("LTC2983");
   spiStart(&SPID1, &hs_spicfg);

   /* Sleep until power up interrupt */
 
   /* Call ltc2983_setup */
  
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
	    err(1);
	}

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
 	
	/* SPI Reccieve*/
	spiReceive(&SPID1, len, data)
	spiUnselect(&SPID1);
}

/* Register Setup */
static void ltc2983_setup(void) {







}
