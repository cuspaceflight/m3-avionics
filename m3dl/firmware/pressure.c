#include "ch.h"
#include "hal.h"

#include <string.h>

#include "logging.h"
#include "pressure.h"
#include "err_handler.h"

#include "m3can.h"
#include "m3status.h"

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


static THD_WORKING_AREA(pressure_wa, 128);
static THD_FUNCTION(pressure_thd, arg) {
    
    /* Set Thread Name */
	(void)arg;
	chRegSetThreadName("Pressure");
    
    /* Start UART Driver */
    uartStart(&UARTD2, &uart_cfg);
    
    while(TRUE) {
    
        chThdSleepMilliseconds(1000);
    
    }
}


/* Entry Point */
void pressure_init(void) {

    /* Create Pressure Thread */
    chThdCreateStatic(pressure_wa, sizeof(pressure_wa),
                  NORMALPRIO, pressure_thd, NULL);
}
