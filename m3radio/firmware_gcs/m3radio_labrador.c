#include <string.h>

#include "labrador.h"
#include "si446x.h"
#include "m3can.h"
#include "m3radio_labrador.h"
#include "ch.h"
#include "chprintf.h"

#define TXCODE LDPC_CODE_N256_K128
#define RXCODE LDPC_CODE_N1280_K1024

static uint8_t labrador_wa[LDPC_SIZE(FAST, TXCODE, MP, RXCODE)];
static uint8_t txbuf[LDPC_PARAM_K(TXCODE)/8];

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
    .gpio0 = 0,
    .gpio1 = LINE_RADIO_GPIO1,
    .gpio2 = 0,
    .gpio3 = 0,
    .nirq = LINE_RADIO_IRQ_N,
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

THD_WORKING_AREA(m3radio_labrador_thd_wa, 1024);
THD_FUNCTION(m3radio_labrador_thd, arg) {
    (void)arg;

    uint8_t* rxbuf;


    /* Initialise Labrador systems */
    while(labrador_init(&labcfg, &labradcfg, &labrador_radio_si446x)
          != LABRADOR_OK)
    {
        palSetLine(LINE_LED_RED);
        chThdSleepMilliseconds(1000);
    }

    /* Initialise the Si446x driver */
    while(!si446x_init(&brdcfg, &labradcfg)) {
        palSetLine(LINE_LED_RED);
        chThdSleepMilliseconds(1000);
    }

    palClearLine(LINE_LED_RED);

    /* Loop sending/receiving messages */
    while (true) {
        /* If there's a packet ready to send,
         * send it and then signal that we've done so.
         */
        chSysLock();
        bool msg_pending = chMsgIsPendingI(m3radio_labrador_thdp);
        chSysUnlock();
        if(msg_pending) {
            chMsgWait();
            uint8_t* txbuf = (uint8_t*)chMsgGet(m3radio_labrador_thdp);
            labrador_err result = labrador_tx(txbuf);
            if(result != LABRADOR_OK) {
                palSetLine(LINE_LED_RED);
            } else {
                palClearLine(LINE_LED_RED);
            }
            chMsgRelease(m3radio_labrador_thdp, (msg_t)result);
        }

        /* Try and receive a message, on success, send it over CAN. */
        labrador_err result = labrador_rx(&rxbuf);
        if(result == LABRADOR_OK) {
            uint8_t n_frames = rxbuf[0];
            uint8_t i, j;
            for(i=0, j=1; i<n_frames; i++) {
                uint16_t sid;
                uint8_t rtr, dlc;
                uint8_t data[8];
                sid = (rxbuf[j+0]<<3) | (rxbuf[j+1] >> 5);
                rtr = rxbuf[j+1] & 0x10;
                dlc = rxbuf[j+1] & 0x0F;
                memcpy(data, &rxbuf[j+2], dlc);
                can_send(sid, rtr, data, dlc);
                j += 2 + dlc;
            }
            palClearLine(LINE_LED_RED);
        } else if(result != LABRADOR_NO_DATA) {
            palSetLine(LINE_LED_RED);
        }
    }
}

void m3radio_labrador_init()
{
    m3radio_labrador_thdp = chThdCreateStatic(
        m3radio_labrador_thd_wa, sizeof(m3radio_labrador_thd_wa),
        NORMALPRIO, m3radio_labrador_thd, NULL);
}

void can_recv(uint16_t msg_id, bool can_rtr, uint8_t* data, uint8_t datalen) {
    txbuf[0]  = (msg_id  >> 3) & 0x00FF;
    txbuf[1]  = (msg_id  << 5) & 0x00E0;
    txbuf[1] |= (can_rtr << 5) & 0x0010;
    txbuf[1] |= (datalen     ) & 0x000F;
    memcpy(&txbuf[2], data, datalen);
    if(m3radio_labrador_thdp != NULL) {
        chMsgSend(m3radio_labrador_thdp, (msg_t)txbuf);
    }
}
