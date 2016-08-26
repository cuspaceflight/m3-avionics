#include "ch.h"
#include "hal.h"

#include "m3can.h"
#include "m3pyro_continuity.h"
#include "m3pyro_arming.h"
#include "m3pyro_firing.h"

int main(void) {
    /* Allow debug access during WFI sleep */
    DBGMCU->CR |= DBGMCU_CR_DBG_SLEEP;

    /* Turn on the watchdog timer, stopped in debug halt */
    DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP;
    IWDG->KR = 0x5555;
    IWDG->PR = 3;
    IWDG->KR = 0xCCCC;

    halInit();
    chSysInit();

    can_init(CAN_ID_M3PYRO);

    palClearLine(LINE_FIRE1);
    palClearLine(LINE_FIRE2);
    palClearLine(LINE_FIRE3);
    palClearLine(LINE_FIRE4);

    m3pyro_continuity_init();
    m3pyro_arming_init();
    m3pyro_firing_init();

    while (true) {
        /* Clear the watchdog timer */
        IWDG->KR = 0xAAAA;

        chThdSleepMilliseconds(100);
    }
}

void can_recv(uint16_t msg_id, bool can_rtr, uint8_t *data, uint8_t datalen) {
    (void)msg_id;
    (void)can_rtr;
    (void)data;
    (void)datalen;

    if((msg_id & 0x1F) != CAN_ID_M3PYRO) {
        return;
    }

    if(msg_id == CAN_MSG_ID_M3PYRO_ARM_COMMAND) {
        uint8_t armed = data[0];
        m3pyro_arming_set(armed);
    } else if(msg_id == CAN_MSG_ID_M3PYRO_FIRE_COMMAND) {
        uint8_t ch1 = data[0], ch2 = data[1], ch3 = data[2], ch4 = data[3];
        m3pyro_firing_fire(ch1, ch2, ch3, ch4);
    }
}
