#include "ch.h"
#include "hal.h"

#include <string.h>
#include "si446x.h"
#include "p_radio.h"
#include "status.h"
#include "labrador.h"
#include "measurements.h"
#include "downlink.h"

/* Primary Radio Config */
struct p_radio_config p_radio_cfg = {
   
    .spid = &SPID1,
    .spi_cfg = {
        .end_cb = NULL,
        .ssport = GPIOA,
        .sspad = GPIOA_PR_NSS,
        .cr1 = SPI_CR1_BR_2,
    },
    .sdn = LINE_PR_SDN,
    .nirq = LINE_PR_NIRQ,
    .gpio0 = si446x_gpio_mode_sync_word_detect,
    .gpio1 = si446x_gpio_mode_tristate,
    .gpio2 = si446x_gpio_mode_tristate,
    .gpio3 = si446x_gpio_mode_tx_state,
    .clk_out_enable = false,
    .clk_out_div = si446x_clk_out_div_1,
    .tcxo = true,
    .xo_freq = 26000000,
    .freq = 869500000,
    .baud = 2000,
    .rxlen = 32,
};

/* Primary Radio Stats */
struct labrador_stats* pr_stats;

/* Thread to Handle Rocket Downlink */
static THD_WORKING_AREA(dwn_thd_wa, 1024);
static THD_FUNCTION(dwn_thd, arg) {

    (void)arg;
    chRegSetThreadName("PR");

    /* Initialise Primary Radio */
    while(!p_radio_init(&p_radio_cfg))
    {
        set_status(COMPONENT_PR, STATUS_ERROR);
        chThdSleepMilliseconds(1000);
    }

    uint8_t rxbuf[64] = {0};
    
    while(true) {
        
        set_status(COMPONENT_PR, STATUS_GOOD);
        
        /* Check for RX */
        if(p_radio_rx(rxbuf, pr_stats)) {
        
            /* Packet Detected */
            set_status(COMPONENT_PR, STATUS_ACTIVITY);                
        }
        
        /* Sleep */
        chThdSleepMilliseconds(50);        
    }
}


void downlink_init(void) {

    chThdCreateStatic(dwn_thd_wa, sizeof(dwn_thd_wa), NORMALPRIO, dwn_thd, NULL);
}
