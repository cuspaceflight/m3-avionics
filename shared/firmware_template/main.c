#include "ch.h"
#include "hal.h"
#include "m3can.h"

int main(void) {

    /* Allow debug access during WFI sleep */
    DBGMCU->CR |= DBGMCU_CR_DBG_SLEEP;

    /* Initialise ChibiOS */
    halInit();
    chSysInit();

    /* Turn on the CAN system and send a packet with our firmware version */
    can_init(CAN_ID_M3TEMPLATE);

    while (true) {
        chThdSleepMilliseconds(100);
    }
}
