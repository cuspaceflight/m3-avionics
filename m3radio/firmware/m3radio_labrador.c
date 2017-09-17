#include <string.h>

#include "labrador.h"
#include "si446x.h"
#include "m3can.h"
#include "m3radio_status.h"
#include "m3radio_labrador.h"
#include "m3radio_router.h"
#include "ch.h"
#include "chprintf.h"

static uint8_t labrador_wa[LABRADOR_WA_SIZE(TM1280, TC128, MS)];
static uint8_t txbuf[128];
BSEMAPHORE_DECL(m3radio_labrador_pps_bsem, true);

/* Board configuration.
 * This tells the Si446x driver what our hardware looks like.
 */
struct si446x_board_config brdcfg = {
    .spid = &SPID2,
    .spi_cfg = {
        .end_cb = NULL,
        .ssport = GPIOA,
        .sspad = GPIOA_RADIO_CS,
        .cr1 = SPI_CR1_BR_2,
    },
    .sdn = LINE_RADIO_SDN,
    .nirq = LINE_RADIO_IRQ_N,
    .gpio0 = si446x_gpio_mode_tristate,
    .gpio1 = si446x_gpio_mode_tristate,
    .gpio2 = si446x_gpio_mode_tristate,
    .gpio3 = si446x_gpio_mode_tristate,
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
    .tx_code = LABRADOR_LDPC_CODE_TM1280,
    .rx_code = LABRADOR_LDPC_CODE_TC128,
    .ldpc_ms_decoder = true,
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

/* Callback for the configuration dumping utility in the Si446x driver */
static void si446x_cfg_cb(uint8_t g, uint8_t p, uint8_t v)
{
    m3can_send_u8(CAN_MSG_ID_M3RADIO_SI4460_CFG, g, p, v, 0, 0, 0, 0, 0, 3);
}

THD_WORKING_AREA(m3radio_labrador_rx_thd_wa, 1024);
THD_FUNCTION(m3radio_labrador_rx_thd, arg) {
    (void)arg;

    uint8_t* rxbuf;

    /* Loop receiving messages */
    while (true) {
        /* Try and receive a message, on success, send it over CAN. */
        labrador_err result = labrador_rx(&rxbuf);
        if(result == LABRADOR_OK) {
            uint16_t sid;
            uint8_t rtr, dlc;
            uint8_t data[8];
            sid = (rxbuf[0]<<3) | (rxbuf[1] >> 5);
            rtr = rxbuf[1] & 0x10;
            dlc = rxbuf[1] & 0x0F;
            memcpy(data, &rxbuf[2], dlc);
            m3can_send(sid, rtr, data, dlc);

            /* Also send updated radio stats based on this packet */
            m3can_send_u32(CAN_MSG_ID_M3RADIO_PACKET_COUNT,
                           labstats.tx_count, labstats.rx_count, 2);
            m3can_send_u16(CAN_MSG_ID_M3RADIO_PACKET_STATS,
                           labstats.rssi, labstats.freq_offset,
                           labstats.n_bit_errs, labstats.ldpc_iters, 4);

            m3status_set_ok(M3RADIO_COMPONENT_LABRADOR);
        } else if(result != LABRADOR_NO_DATA) {
            m3status_set_error(M3RADIO_COMPONENT_LABRADOR,
                               M3RADIO_ERROR_LABRADOR_RX);
        }

        chThdSleepMilliseconds(10);
    }
}


THD_WORKING_AREA(m3radio_labrador_tx_thd_wa, 1024);
THD_FUNCTION(m3radio_labrador_tx_thd, arg) {
    (void)arg;

    /* Loop transmitting messages */
    while(true) {
        /* GPS PPS will fall low 20ms before top of second, so we wait
         * for that, which gives us time to prepare before it goes high
         * and triggers transmission. We timeout after 1s in case the PPS
         * has stopped, so that we'll still transmit anyway.
         */
        chBSemWaitTimeout(&m3radio_labrador_pps_bsem, MS2ST(1000));
        m3radio_router_fillbuf(txbuf, sizeof(txbuf));
        labrador_err result = labrador_tx(txbuf);
        if(result != LABRADOR_OK) {
            m3status_set_error(M3RADIO_COMPONENT_LABRADOR,
                               M3RADIO_ERROR_LABRADOR_TX);
        } else {
            m3status_set_ok(M3RADIO_COMPONENT_LABRADOR);
        }
    }
}

void m3radio_labrador_init()
{
    m3status_set_init(M3RADIO_COMPONENT_LABRADOR);

    /* Initialise Labrador systems */
    while(labrador_init(&labcfg, &labradcfg, &labstats, &labrador_radio_si446x)
          != LABRADOR_OK)
    {
        m3status_set_error(M3RADIO_COMPONENT_LABRADOR,
                           M3RADIO_ERROR_LABRADOR);

        chThdSleepMilliseconds(1000);
    }

    /* Initialise the Si446x driver */
    while(!si446x_init(&brdcfg, &labradcfg)) {
        m3status_set_error(M3RADIO_COMPONENT_LABRADOR,
                           M3RADIO_ERROR_LABRADOR_SI4460);

        chThdSleepMilliseconds(1000);
    }

    /* Dump the Si446x configuration to CAN for logging */
    si446x_dump_params(si446x_cfg_cb);

    /* Start RX thread */
    chThdCreateStatic(
        m3radio_labrador_rx_thd_wa, sizeof(m3radio_labrador_rx_thd_wa),
        NORMALPRIO, m3radio_labrador_rx_thd, NULL);

    /* Start TX thread */
    chThdCreateStatic(
        m3radio_labrador_tx_thd_wa, sizeof(m3radio_labrador_tx_thd_wa),
        NORMALPRIO, m3radio_labrador_tx_thd, NULL);
}

void m3radio_labrador_pps_falling(EXTDriver *extp, expchannel_t channel)
{
    (void)extp;
    (void)channel;
    chSysLockFromISR();
    chBSemSignalI(&m3radio_labrador_pps_bsem);
    chSysUnlockFromISR();
}
