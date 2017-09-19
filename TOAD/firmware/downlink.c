#include "ch.h"
#include "hal.h"

#include <string.h>
#include "si446x.h"
#include "si446x_toad_pr.h"
#include "status.h"
#include "labrador.h"
#include "measurements.h"
#include "downlink.h"
#include "logging.h"

/* Primary Radio Config */
static struct si446x_board_config brdcfg = {
    .spid = &SPID1,
    .spi_cfg = {
        .end_cb = NULL,
        .ssport = GPIOA,
        .sspad = GPIOA_PR_NSS,
        .cr1 = SPI_CR1_BR_2,
    },
    .sdn = LINE_PR_SDN,
    .nirq = LINE_PR_NIRQ,
    .gpio0 = si446x_gpio_mode_rx_data_clk,
    .gpio1 = si446x_gpio_mode_tristate,
    .gpio2 = si446x_gpio_mode_tristate,
    .gpio3 = si446x_gpio_mode_tx_state,
    .clk_out_enable = false,
    .clk_out_div = si446x_clk_out_div_1,
    .tcxo = true,
    .xo_freq = 26000000,
};

static struct labrador_radio_config labcfg = {
    .freq = 869500000,
    .baud = 2000,
    .rxlen = 160,
    .txlen = 160,
    .rx_enabled = true,
};

/* Primary Radio Stats */
static struct labrador_stats* pr_stats;

/* Thread to Handle Rocket Downlink */
static THD_WORKING_AREA(dwn_thd_wa, 1024);
static THD_FUNCTION(dwn_thd, arg) {

    (void)arg;
    chRegSetThreadName("PR");

    /* Initialise Primary Radio */
    while(!si446x_toad_pr_init(&brdcfg, &labcfg))
    {
        set_status(COMPONENT_PR, STATUS_ERROR);
        chThdSleepMilliseconds(1000);
    }

    uint8_t rxbuf[160] = {0};

    while(true) {

        set_status(COMPONENT_PR, STATUS_GOOD);

        /* Check for Telemetry Packet */
        if(si446x_toad_pr_rx(rxbuf, pr_stats)) {

            /* Packet Recieved */
            set_status(COMPONENT_PR, STATUS_ACTIVITY);

            /* Log Telem Packet */
            log_telem_packet(rxbuf);
            log_labrador_stats(pr_stats);
        }

        /* Sleep */
        chThdSleepMilliseconds(10);
    }
}


void downlink_init(void) {

    chThdCreateStatic(dwn_thd_wa, sizeof(dwn_thd_wa), NORMALPRIO, dwn_thd, NULL);
}
