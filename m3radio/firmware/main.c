#include "ch.h"
#include "hal.h"
#include "m3can.h"
#include "m3radio_status.h"
#include "ublox.h"

int main(void) {

    /* Allow debug access during WFI sleep */
    DBGMCU->CR |= DBGMCU_CR_DBG_SLEEP;

    /* Turn on the watchdog timer, stopped in debug halt */
    DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP;
    IWDG->KR = 0x5555;
    IWDG->PR = 3;
    IWDG->KR = 0xCCCC;

    /* Initialise ChibiOS */
    halInit();
    chSysInit();

    /* Turn on the CAN system and send a packet with our firmware version */
    can_init(CAN_ID_M3RADIO);

    uart_init(UARTD4);

    m3radio_status_init();

    while (true) {
        /* Clear the watchdog timer */
        IWDG->KR = 0xAAAA;

        chThdSleepMilliseconds(100);
    }
}
