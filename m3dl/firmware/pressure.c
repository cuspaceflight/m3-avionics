#include "ch.h"
#include "hal.h"

#include <string.h>

#include "logging.h"
#include "pressure.h"
#include "err_handler.h"

#include "m3can.h"
#include "m3status.h"

/* Function Prototypes */
bool process_buffer(void);
static uint16_t compute_crc(uint8_t *buf, size_t len);

/* UART RX Buffer */
static uint8_t rxbuf[20];
static systime_t last_reception = 0;


/* UART2 Config */
static const UARTConfig uart_cfg = {
    .txend1_cb = NULL,
    .txend2_cb = NULL,
    .rxend_cb  = NULL,
    .rxchar_cb = NULL,
    .rxerr_cb  = NULL,
    .speed     = 38400,
    .cr1       = 0,
    .cr2       = 0,
    .cr3       = 0,
};


/* Main Thread */
static THD_WORKING_AREA(pressure_wa, 256);
static THD_FUNCTION(pressure_thd, arg) {
    
    /* Set Thread Name */
	(void)arg;
	chRegSetThreadName("Pressure");
    
    /* Start UART Driver */
    uartStart(&UARTD2, &uart_cfg);
    
    while(TRUE) {
    
        /* Get Current Time */
        last_reception = chVTGetSystemTimeX();
        
        /* Recieve Buffer */
        size_t n = sizeof(rxbuf);
        uartReceiveTimeout(&UARTD2, &n, &rxbuf, TIME_INFINITE);
        
        /* Check for Timeout */
        if(ST2MS(chVTTimeElapsedSinceX(last_reception)) > 500) {
            m3status_set_error(M3DL_COMPONENT_PRESSURE, M3DL_ERROR_PRESSURE_TIMEOUT);
            err(M3DL_ERROR_PRESSURE_TIMEOUT);
        }
        
        /* Process Buffer */
        if(process_buffer()) {
             m3status_set_ok(M3DL_COMPONENT_PRESSURE);
        } else {
            m3status_set_error(M3DL_COMPONENT_PRESSURE, M3DL_ERROR_CRC_FAILED);
            err(M3DL_ERROR_CRC_FAILED);
        }
    }
}


/* Entry Point */
void pressure_init(void) {

    /* Create Pressure Thread */
    chThdCreateStatic(pressure_wa, sizeof(pressure_wa),
                  NORMALPRIO, pressure_thd, NULL);
}


/* Process RX Buffer */
bool process_buffer(void) {

    uint8_t res[10] = {0};
    uint8_t b = 0;
    uint16_t chk;
    size_t m = sizeof(res);
    size_t j = 0;
    size_t i = 0;
    
    /* Fill Results Buffer */
    while(j < m) {
        
        /* Read in First Byte */
        b = rxbuf[i];
        
        /* Check for Start Byte */
        if(b == 0x7E) {
            
            /* Do Nothing */
            j = 0;
            i = i+1;    
            
        } else {
            
            /* Handle Escaped Characters */
            if(b == 0x7D) {
                b = rxbuf[i+1] ^ 0x20;
                i = i+2;
            } else {
                i = i+1;
            }
            
            /* Write Byte to Results Buffer */
            res[j] = b;
            j = j+1;
        }
    }

    /* Log Pressure Readings */
    m3can_send(CAN_MSG_ID_M3DL_PRESSURE, FALSE, res, 8);

    /* Compute Local CRC */
    chk = compute_crc(res, 8);
    
    /* Compare CRCs */
    if(((chk & 0xFF) == res[m-2]) && (((chk >> 8) & 0xFF) == res[m-1])) {
        return true;
    } else {
        return false;
    }
}


/* Compute CRC16 */
static uint16_t compute_crc(uint8_t *buf, size_t len) {
    size_t i, j;
    uint16_t crc = 0xFFFF;
    for(i=0; i<len; i++) {
        crc ^= (uint16_t) buf[i] << 8;
        for(j=0; j<8; j++) {
            if(crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc ^ 0xFFFF;
}
