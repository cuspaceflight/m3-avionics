#include "ch.h"
#include "hal.h"
#include "m3can.h"
#include "m3radio_status.h"
#include "m3radio_gps_ant.h"
#include "ublox.h"

#include <string.h>

#include "radios/si446x.h"

THD_WORKING_AREA(tmpwa, 512);
THD_FUNCTION(radthd, arg) {
    (void)arg;
    struct si446x_board_config brdcfg = {
        .spid = &SPID2,
        .spi_cfg = {
            .end_cb = NULL,
            .ssport = GPIOB,
            .sspad = GPIOB_RADIO_CS,
            .cr1 = SPI_CR1_BR_2,
        },
        .sdn = LINE_RADIO_SDN,
        .tcxo = true,
        .xo_freq = 26000000,
    };

    struct labrador_radio_config labcfg = {
        .freq = 869500000,
        .baud = 2000,
        .txlen = 32,
        .rxlen = 32,
    };

    uint8_t rxbuf[32] = {0};

    m3status_set_init(M3RADIO_COMPONENT_SI4460);

    bool rv = si446x_init(&brdcfg, &labcfg);

    if(!rv) {
        m3status_set_error(M3RADIO_COMPONENT_SI4460, M3RADIO_ERROR_SI4460_CFG);
        while(true) {
            chThdSleepMilliseconds(100);
        }
    }

    while (true) {
        m3status_set_ok(M3RADIO_COMPONENT_SI4460);
        memset(rxbuf, 0, sizeof(rxbuf));
        rv = si446x_rx(rxbuf);
        chThdSleepMilliseconds(100);
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

    /* Initialise ChibiOS */
    halInit();
    chSysInit();

    /* Turn on the CAN system and send a packet with our firmware version */
    can_init(CAN_ID_M3RADIO);

    /* We'll enable CAN loopback so we can send our own messages over
     * the radio */
    can_set_loopback(true);

    m3radio_status_init();

    m3radio_gps_ant_init();
    ublox_init(&SD4);

#if 0
    struct si4460_config si4460_cfg = {
        .spid = &SPID2,
        .spi_cfg = {
            .end_cb = NULL,
            .ssport = GPIOB,
            .sspad = GPIOB_RADIO_CS,
            .cr1 = SPI_CR1_BR_2,
        },
        .sdn = true,
        .sdnline = LINE_RADIO_SDN,
        .tcxo = true,
        .xo_freq = 26000000,
        .centre_freq = 869500000,
    };
#endif

    chThdCreateStatic(tmpwa, sizeof(tmpwa), NORMALPRIO, radthd, NULL);

    while (true) {
        /* Clear the watchdog timer */
        IWDG->KR = 0xAAAA;

        chThdSleepMilliseconds(100);
    }
}
