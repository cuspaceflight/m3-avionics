
#include "ch.h"
#include "hal.h"

#include "psu.h"

int main(void) {

    /* Allow debug access during WFI sleep */
    DBGMCU->CR |= DBGMCU_CR_DBG_SLEEP;

    /* Initialise ChibiOS */
    halInit();
    chSysInit();
    
    /* Do some stuff */
    psu_init();

}
