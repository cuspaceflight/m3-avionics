#include <string.h>

#include "labrador.h"
#include "si446x.h"
#include "m3can.h"
#include "m3radio_status.h"
#include "m3radio_labrador.h"
#include "ch.h"
#include "chprintf.h"

/* Remember to update m3radio_labrador.h:M3RADIO_LABRADOR_TXBUFSIZE */
#define TXCODE LDPC_CODE_N1280_K1024
#define RXCODE LDPC_CODE_N256_K128

static uint8_t labrador_wa[LDPC_SIZE(FAST, TXCODE, MP, RXCODE)];

static thread_t* m3radio_labrador_thdp = NULL;

/* Board configuration.
 * This tells the Si446x driver what our hardware looks like.
 */
struct si446x_board_config brdcfg = {
    .spid = &SPID2,
    .spi_cfg = {
        .end_cb = NULL,
        .ssport = GPIOB,
        .sspad = GPIOB_RADIO_CS,
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

/* Callback for the configuration dumping utility in the Si446x driver */
static void si446x_cfg_cb(uint8_t g, uint8_t p, uint8_t v)
{
    m3can_send_u8(CAN_MSG_ID_M3RADIO_SI4460_CFG, g, p, v, 0, 0, 0, 0, 0, 3);
}

THD_WORKING_AREA(m3radio_labrador_thd_wa, 1024);
THD_FUNCTION(m3radio_labrador_thd, arg) {
    (void)arg;

    uint8_t* rxbuf;

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

    /* Loop sending/receiving messages */
    while (true) {
        /* If there's a packet ready to send,
         * send it and then signal that we've done so.
         */
        chSysLock();
        bool msg_pending = chMsgIsPendingI(m3radio_labrador_thdp);
        chSysUnlock();
        if(msg_pending) {
            thread_t *tp = chMsgWait();
            uint8_t* txbuf = (uint8_t*)chMsgGet(tp);
            labrador_err result = labrador_tx(txbuf);
            if(result != LABRADOR_OK) {
                m3status_set_error(M3RADIO_COMPONENT_LABRADOR,
                                   M3RADIO_ERROR_LABRADOR_TX);
            } else {
                m3status_set_ok(M3RADIO_COMPONENT_LABRADOR);
            }
            chMsgRelease(tp, (msg_t)result);
        }

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
    }
}

void m3radio_labrador_tx(uint8_t* buf)
{
    if(m3radio_labrador_thdp != NULL) {
        chMsgSend(m3radio_labrador_thdp, (msg_t)buf);
    }
}

void m3radio_labrador_init()
{
    m3radio_labrador_thdp = chThdCreateStatic(
        m3radio_labrador_thd_wa, sizeof(m3radio_labrador_thd_wa),
        NORMALPRIO, m3radio_labrador_thd, NULL);
}
