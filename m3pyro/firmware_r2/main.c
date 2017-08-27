#include "ch.h"
#include "hal.h"

#include "m3can.h"
#include "m3pyro_status.h"
#include "m3pyro_hal.h"
#include "m3pyro_selftest.h"
#include "m3pyro_continuity.h"
#include "m3pyro_firing.h"

static THD_WORKING_AREA(monitor_thd_wa, 128);
static THD_FUNCTION(monitor_thd, arg) {
    (void)arg;
    chRegSetThreadName("monitor");
    uint8_t readings[2];
    while(true) {
        readings[0] = m3pyro_read_supply();
        readings[1] = m3pyro_read_bus();
        m3can_send(CAN_MSG_ID_M3PYRO_SUPPLY_STATUS, false, readings, 2);
        chThdSleepMilliseconds(500);
    }
}

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

    uint16_t filter_ids[] = { CAN_ID_M3PYRO };
    m3can_init(CAN_ID_M3PYRO, filter_ids, 1);

    m3pyro_status_init();
    m3pyro_hal_init();

    m3pyro_disarm();

    /* Run the self test routine, which will update m3status
     * on success/failure. We still proceed with normal
     * operation even on self test failure, because not
     * all self test failures are critical failures.
     */
    m3pyro_selftest();

    /* Start the supply and bus voltage monitor thread. */
    chThdCreateStatic(monitor_thd_wa, sizeof(monitor_thd_wa),
                      LOWPRIO, monitor_thd, NULL);

    /* Start the continuity measurement thread. */
    m3pyro_continuity_init();

    /* Start the firing thread. */
    m3pyro_firing_init();

    /* Main thread can just idle clearing watchdog now. */
    while (true) {
        /* Clear the watchdog timer */
        IWDG->KR = 0xAAAA;

        chThdSleepMilliseconds(100);
    }
}

void m3can_recv(uint16_t msg_id, bool rtr, uint8_t *data, uint8_t datalen) {
    (void)msg_id;
    (void)rtr;

    /* Only respond to CAN messages addressed to M3Pyro */
    if((msg_id & 0x1F) != CAN_ID_M3PYRO) {
        return;
    }

    if(msg_id == CAN_MSG_ID_M3PYRO_ARM_COMMAND) {
        /* Handle arming/disarming command */
        uint8_t armed = data[0];
        if(armed) {
            m3pyro_arm();
        } else {
            m3pyro_disarm();
        }
    } else if(msg_id == CAN_MSG_ID_M3PYRO_FIRE_COMMAND) {
        /* Handle fire command */
        uint8_t ch1 = data[0], ch2 = data[1], ch3 = data[2], ch4 = data[3];
        uint8_t ch5 = data[4], ch6 = data[5], ch7 = data[6], ch8 = data[7];

        if(datalen >= 1 && ch1 != 0) {
            m3pyro_firing_enqueue(1, ch1);
        }

        if(datalen >= 2 && ch2 != 0) {
            m3pyro_firing_enqueue(2, ch2);
        }

        if(datalen >= 3 && ch3 != 0) {
            m3pyro_firing_enqueue(3, ch3);
        }

        if(datalen >= 4 && ch4 != 0) {
            m3pyro_firing_enqueue(4, ch4);
        }

        if(datalen >= 5 && ch5 != 0) {
            m3pyro_firing_enqueue(5, ch5);
        }

        if(datalen >= 6 && ch6 != 0) {
            m3pyro_firing_enqueue(6, ch6);
        }

        if(datalen >= 7 && ch7 != 0) {
            m3pyro_firing_enqueue(7, ch7);
        }

        if(datalen >= 8 && ch8 != 0) {
            m3pyro_firing_enqueue(8, ch8);
        }
    }
}
