#include "ch.h"
#include "hal.h"

#include <string.h>
#include "labrador.h"
#include "si446x.h"
#include "s_radio.h"
#include "status.h"
#include "measurements.h"


#define TXCODE LDPC_CODE_N512_K256
#define RXCODE LDPC_CODE_N512_K256

static uint8_t labrador_wa[LDPC_SIZE(FAST, TXCODE, MP, RXCODE)];

static thread_t* sr_labrador_thdp = NULL;

/* Board configuration.
 * This tells the Si446x driver what our hardware looks like.
 */
struct si446x_board_config brdcfg = {
    .spid = &SPID2,
    .spi_cfg = {
        .end_cb = NULL,
        .ssport = GPIOA,
        .sspad = GPIOA_SR_NSS,
        .cr1 = SPI_CR1_BR_2,
    },
    .sdn = LINE_SR_SDN,
    .nirq = LINE_SR_NIRQ,
    .gpio0 = si446x_gpio_mode_tristate,
    .gpio1 = si446x_gpio_mode_tristate,
    .gpio2 = si446x_gpio_mode_tristate,
    .gpio3 = si446x_gpio_mode_tx_state,
    .clk_out_enable = false,
    .clk_out_div = si446x_clk_out_div_1,
    .tcxo = true,
    .xo_freq = 26000000,
};


/* Labrador configuration.
 * This specifies our Labrador frequency, baud, codes, encoder/decoder, etc.
 */
struct labrador_config labcfg = {
    .freq = 869500000,
    .baud = 2000,
    .tx_code = TXCODE,
    .rx_code = RXCODE,
    .ldpc_none_txlen = 0,
    .ldpc_none_rxlen = 0,
    .ldpc_fast_encoder = true,
    .ldpc_mp_decoder = true,
    .rx_enabled = true,
    .workingarea = labrador_wa,
    .workingarea_size = sizeof(labrador_wa),
};


/* Labrador radio config. Passed between Labrador and Si446x driver.
 */
struct labrador_radio_config labradcfg;


/* Labrador statistics. Updated by Labrador, read by us.
 */
struct labrador_stats labstats;


/* Thread to Handle Secondary Radio */
static THD_WORKING_AREA(sr_thd_wa, 1024);
static THD_FUNCTION(sr_thd, arg) {

    (void)arg;
    chRegSetThreadName("SR");

    /* Initialise Labrador systems */
    while(labrador_init(&labcfg, &labradcfg, &labstats, &labrador_radio_si446x)
          != LABRADOR_OK)
    {
        set_status(COMPONENT_SR, STATUS_ERROR);
        chThdSleepMilliseconds(1000);
    }

    /* Initialise the Si446x driver */
    while(!si446x_init(&brdcfg, &labradcfg)) {
    
        set_status(COMPONENT_SR, STATUS_ERROR);
        chThdSleepMilliseconds(1000);
    }

    /* Loop sending messages */
    while (true) {
        
        /* If there's a packet ready to send,
         * send it and then signal that we've done so.
         */
        chSysLock();
        bool msg_pending = chMsgIsPendingI(sr_labrador_thdp);
        chSysUnlock();
        if(msg_pending) {
            thread_t *tp = chMsgWait();
            uint8_t* txbuf = (uint8_t*)chMsgGet(tp);
            labrador_err result = labrador_tx(txbuf);
            if(result != LABRADOR_OK) {
                set_status(COMPONENT_SR, STATUS_ERROR);
            } else {
                set_status(COMPONENT_SR, STATUS_GOOD);
            }
            chMsgRelease(tp, (msg_t)result);
        }
    }  
}


/* Transmit Data */
void sr_labrador_tx(uint8_t* buf)
{
    if(sr_labrador_thdp != NULL) {
        chMsgSend(sr_labrador_thdp, (msg_t)buf);
    }
}


/* Start Secondary Radio Thread */
void sr_labrador_init(void) {

    sr_labrador_thdp = chThdCreateStatic(sr_thd_wa, sizeof(sr_thd_wa), NORMALPRIO, sr_thd, NULL);
}

