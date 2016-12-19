#include "ch.h"
#include "hal.h"

#include <string.h>

#include "LTC2983.h"
#include "logging.h"
#include "err_handler.h"

#include "m3can.h"
#include "m3status.h"


/* Conversion Status Semaphore */
binary_semaphore_t temp_ready_sem;

/* Temperature Results Buffer */	
static uint8_t temp_results[80] = {0};

/* Function Prototypes */
static void ltc2983_write_reg(uint16_t addr, size_t len, uint8_t* data);
static void ltc2983_read_reg(uint16_t addr, size_t len, uint8_t* data);
static void ltc2983_setup(void);
static void log_temp(void);

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

	/* Signal Semaphore */
	chSysLockFromISR();
	chBSemSignalI(&temp_ready_sem);
	chSysUnlockFromISR();

}

/* Temperature Sensing Thread */
static THD_WORKING_AREA(ltc2983_wa, 256);
static THD_FUNCTION(ltc2983_thd, arg) {
	
	/* Set Thread Name */
	(void)arg;
	chRegSetThreadName("LTC2983");
	
	/* Start SPI Driver */
	spiStart(&SPID1, &hs_spicfg);
 	
	/* Call ltc2983_setup function */
	ltc2983_setup();
	
	/* Setup succesful */
	m3status_set_ok(M3DL_COMPONENT_LTC2983);

	/* Initiate Continious Multi-Channel Conversion */
	while(TRUE) {

	    /* Initiate Conversion */
	    uint8_t cmd_init = 0x80;

	    ltc2983_write_reg(CMD_STATUS_REG, 1, &cmd_init);

	    /* Wait for Conversion to Complete */
	    chBSemWait(&temp_ready_sem);
	    
	    /* Check In */
	    m3status_set_ok(M3DL_COMPONENT_LTC2983);
	
	    /* Read Results */
	    ltc2983_read_reg(TEMP_RES_REG, 80, (uint8_t *)(temp_results));
	    
	    /* Log Results */
	    log_temp();
	    
	}
  
}

/* Entry Point */
void ltc2983_init(void) {
    
    /* Init Temp Ready Semaphore */
    chBSemObjectInit(&temp_ready_sem, FALSE);

    /* Create LTC2983 Thread */
    chThdCreateStatic(ltc2983_wa, sizeof(ltc2983_wa),
                  NORMALPRIO, ltc2983_thd, NULL);

}


/* Write to LTC2983 */
static void ltc2983_write_reg(uint16_t addr, size_t len, uint8_t* data) {
	
	/* Setup TX Buffer */
	static uint8_t txbuf[83];

	/* Check Data Length */
	if (len > 80) {
	    err(M3DL_ERROR_LTC2983_OVERFLOW);
	    m3status_set_error(M3DL_COMPONENT_LTC2983, M3DL_ERROR_LTC2983_OVERFLOW);
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

/* Log Temperature Data */
static void log_temp(void) {

    uint8_t TEMP_1_2[8] = {0};
    uint8_t TEMP_3_4[8] = {0};
    uint8_t TEMP_5_6[8] = {0};
    uint8_t TEMP_7_8[8] = {0};
    
    /* Populate Results Buffers */
    memcpy((uint8_t*)(TEMP_1_2), (uint8_t*)(temp_results + 4), 4);
    memcpy((uint8_t*)(TEMP_1_2 + 4), (uint8_t*)(temp_results + 12), 4);
    memcpy((uint8_t*)(TEMP_3_4), (uint8_t*)(temp_results + 20), 4);
    memcpy((uint8_t*)(TEMP_3_4 + 4), (uint8_t*)(temp_results + 28), 4);
    memcpy((uint8_t*)(TEMP_5_6), (uint8_t*)(temp_results + 36), 4);
    memcpy((uint8_t*)(TEMP_5_6 + 4), (uint8_t*)(temp_results + 44), 4);
    memcpy((uint8_t*)(TEMP_7_8), (uint8_t*)(temp_results + 52), 4);
    memcpy((uint8_t*)(TEMP_7_8 + 4), (uint8_t*)(temp_results + 60), 4);
    
     
    /* Test for Validity */   
    if(TEMP1_ATTACHED == 1) {
        if(!(&TEMP_1_2[0] && 0x01)) {
            m3status_set_error(M3DL_COMPONENT_LTC2983, M3DL_ERROR_TEMP1_INVALID);
            err(M3DL_ERROR_TEMP1_INVALID);
        }
    }

    if(TEMP2_ATTACHED == 1) {
        if(!(&TEMP_1_2[4] && 0x01)) {
            m3status_set_error(M3DL_COMPONENT_LTC2983, M3DL_ERROR_TEMP2_INVALID);
            err(M3DL_ERROR_TEMP2_INVALID);
        }
    }

    if(TEMP3_ATTACHED == 1) {
        if(!(&TEMP_3_4[0] && 0x01)) {
            m3status_set_error(M3DL_COMPONENT_LTC2983, M3DL_ERROR_TEMP3_INVALID);
            err(M3DL_ERROR_TEMP3_INVALID);
        }
    }

    if(TEMP4_ATTACHED == 1) {
        if(!(&TEMP_3_4[4] && 0x01)) {
            m3status_set_error(M3DL_COMPONENT_LTC2983, M3DL_ERROR_TEMP4_INVALID);
            err(M3DL_ERROR_TEMP4_INVALID);
        }
    }

    if(TEMP5_ATTACHED == 1) {
        if(!(&TEMP_5_6[0] && 0x01)) {
            m3status_set_error(M3DL_COMPONENT_LTC2983, M3DL_ERROR_TEMP5_INVALID);
            err(M3DL_ERROR_TEMP5_INVALID);
        }
    }

    if(TEMP6_ATTACHED == 1) {
        if(!(&TEMP_5_6[4] && 0x01)){
            m3status_set_error(M3DL_COMPONENT_LTC2983, M3DL_ERROR_TEMP6_INVALID);
            err(M3DL_ERROR_TEMP6_INVALID);
        }
    }

    if(TEMP7_ATTACHED == 1) {
        if(!(&TEMP_7_8[0] && 0x01)) {
            m3status_set_error(M3DL_COMPONENT_LTC2983, M3DL_ERROR_TEMP7_INVALID);
            err(M3DL_ERROR_TEMP7_INVALID);
        }
    }

    if(TEMP8_ATTACHED == 1) {
        if(!(&TEMP_7_8[4] && 0x01)) {
            m3status_set_error(M3DL_COMPONENT_LTC2983, M3DL_ERROR_TEMP8_INVALID);
            err(M3DL_ERROR_TEMP8_INVALID);
        }
    }
    
    /* Send TEMP1 & TEMP2 */
    can_send(CAN_MSG_ID_M3DL_TEMP_1_2, FALSE, TEMP_1_2, 8);
    
    /* Send TEMP3 & TEMP4 */
    can_send(CAN_MSG_ID_M3DL_TEMP_3_4, FALSE, TEMP_3_4, 8);
    
    /* Send TEMP5 & TEMP6 */
    can_send(CAN_MSG_ID_M3DL_TEMP_5_6, FALSE, TEMP_5_6, 8);
    
    /* Send TEMP7 & TEMP8 */
    can_send(CAN_MSG_ID_M3DL_TEMP_7_8, FALSE, TEMP_7_8, 8);
}


/* Register Setup */
static void ltc2983_setup(void) {

    /* Allow Power Up Time */
	chThdSleepMilliseconds(200);
	
	/* Read Command Status Register */
	uint8_t cmd_status_reg = 0x00;
	ltc2983_read_reg(CMD_STATUS_REG, 1, &cmd_status_reg);
	
	while(cmd_status_reg != 0x40) {

        /* Signal Error & Re-attempt every 100ms */        
        err(M3DL_ERROR_LTC2983_SETUP);
	    m3status_set_error(M3DL_COMPONENT_LTC2983, M3DL_ERROR_LTC2983_SETUP);
        chThdSleepMilliseconds(100);
        ltc2983_read_reg(CMD_STATUS_REG, 1, &cmd_status_reg);
    }
		
    /* Buffer to Hold Sensor Config Data */
	static uint32_t sensor_config[20] = {0};

    /* Populate Thermocouple Specific Data */
    for (int i = 0; i < 19; i++) {
		if ((i%2) == 1){
			sensor_config[i] = 0x00001015;				
		}
	}
	
	/* Populate Diode Specific Data */
	sensor_config[19] = 0x0000C0E7;
	
	/* Send over SPI */
	ltc2983_write_reg(CHANNEL_ASSIGN_REG, 80, (uint8_t *)sensor_config);
	
	
	/* Multi-Channel Conversion Mask */	 
    uint8_t conversion_mask[3] = {0};
    
    /* TEMP1 - TEMP2 - TEMP3 - TEMP4 */
    if (TEMP1_ATTACHED == 1) {
        conversion_mask[2] |= TEMP1_MASK;
        }
    if (TEMP2_ATTACHED == 1) {
        conversion_mask[2] |= TEMP2_MASK;
        }
    if (TEMP3_ATTACHED == 1) {
        conversion_mask[2] |= TEMP3_MASK;
        }
    if (TEMP4_ATTACHED == 1) {
        conversion_mask[2] |= TEMP4_MASK;
        }
    
    /* TEMP5 - TEMP6 - TEMP7 - TEMP8 */
    if (TEMP5_ATTACHED == 1) {
        conversion_mask[1] |= TEMP5_MASK;
        }
    if (TEMP6_ATTACHED == 1) {
        conversion_mask[1] |= TEMP6_MASK;
        }
    if (TEMP7_ATTACHED == 1) {
        conversion_mask[1] |= TEMP7_MASK;
        }
    if (TEMP8_ATTACHED == 1) {
        conversion_mask[1] |= TEMP8_MASK;
        }

	/* TEMP9 */
	if (TEMP9_ATTACHED == 1) {
        conversion_mask[0] |= TEMP9_MASK;
        }
	
	/* Write Mask Over SPI */
	ltc2983_write_reg(CHANNEL_MASK_REG, 3, conversion_mask);

}

