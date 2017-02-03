#include <string.h>

#include "ch.h"
#include "hal.h"

#include "lab01_labrador.h"
#include "labrador.h"
#include "si446x.h"
#include "usbserial.h"

#define CAN_MSG_ID(x) (x<<5)
#define CAN_ID_GROUND (7)
#define CAN_MSG_ID_GROUND_PACKET_COUNT (CAN_ID_GROUND | CAN_MSG_ID(53))
#define CAN_MSG_ID_GROUND_PACKET_STATS (CAN_ID_GROUND | CAN_MSG_ID(54))

#define TXCODE LDPC_CODE_N256_K128
#define RXCODE LDPC_CODE_N1280_K1024

static uint8_t labrador_wa[LDPC_SIZE(FAST, TXCODE, BF, RXCODE)];
static uint8_t txbuf[LDPC_PARAM_K(TXCODE)/8];

static thread_t* labrador_thdp = NULL;

struct si446x_board_config brdcfg = {
    .spid = &SPID1,
    .spi_cfg = {
        .end_cb = NULL,
        .ssport = PAL_PORT(LINE_SI_CS),
        .sspad = PAL_PAD(LINE_SI_CS),
        .cr1 = SPI_CR1_BR_2,
    },
    .sdn = LINE_SI_SDN,
    .nirq = LINE_SI_IRQ,
    .gpio0 = si446x_gpio_mode_tristate,
    .gpio1 = si446x_gpio_mode_div_clk,
    .gpio2 = si446x_gpio_mode_tristate,
    .gpio3 = si446x_gpio_mode_tx_state,
    .clk_out_enable = true,
    .clk_out_div = si446x_clk_out_div_2,
    .tcxo = true,
    .xo_freq = 26000000,
};

struct labrador_config labcfg = {
    .freq = 869500000,
    .baud = 2000,
    .tx_code = TXCODE,
    .rx_code = RXCODE,
    .ldpc_fast_encoder = true,
    .ldpc_mp_decoder = false,
    .rx_enabled = true,
    .workingarea = labrador_wa,
    .workingarea_size = sizeof(labrador_wa),
};

struct labrador_radio_config labradcfg;
struct labrador_stats labstats;

static THD_WORKING_AREA(lab01_labrador_thd_wa, 1024);
static THD_FUNCTION(lab01_labrador_thd, arg) {
    (void)arg;
    uint8_t* rxbuf;

    while(true) {
        /* If there's a packet ready to send,
         * send it and then signal that we've done so.
         */
        chSysLock();
        bool msg_pending = chMsgIsPendingI(labrador_thdp);
        chSysUnlock();
        if(msg_pending) {
            thread_t *tp = chMsgWait();
            uint8_t* txbuf = (uint8_t*)chMsgGet(tp);
            labrador_err result = labrador_tx(txbuf);
            if(result != LABRADOR_OK) {
                palSetLine(LINE_PIO0);
            } else {
                palClearLine(LINE_PIO0);
            }
            chMsgRelease(tp, (msg_t)result);
        }

        /* Try and receive a message, on success, send it over USB. */
        labrador_err result = labrador_rx(&rxbuf);
        if(result == LABRADOR_OK) {
            palClearLine(LINE_PIO0);
            palSetLine(LINE_PIO1);
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
                usbserial_send(sid, rtr, data, dlc);
                j += 2 + dlc;
            }

            /* additionally send the computer our stats from this packet */
            uint32_t d0 = labstats.tx_count, d1 = labstats.rx_count;
            uint8_t count_data[8] = { d0, d0>>8, d0>>16, d0>>24,
                                      d1, d1>>8, d1>>16, d1>>24 };
            uint16_t d2 = labstats.rssi, d3 = labstats.freq_offset,
                     d4 = labstats.n_bit_errs, d5 = labstats.ldpc_iters;
            uint8_t stats_data[8] = { d2, d2>>8, d3, d3>>8,
                                      d4, d4>>8, d5, d5>>8 };
            usbserial_send(CAN_MSG_ID_GROUND_PACKET_COUNT, 0, count_data, 8);
            usbserial_send(CAN_MSG_ID_GROUND_PACKET_STATS, 0, stats_data, 8);
            palClearLine(LINE_PIO1);
        } else if(result != LABRADOR_NO_DATA) {
            palSetLine(LINE_PIO0);
        }
    }
}

void lab01_labrador_init()
{
    while(labrador_init(&labcfg, &labradcfg, &labstats, &labrador_radio_si446x)
          != LABRADOR_OK)
    {
        palSetLine(LINE_PIO0);
        chThdSleepMilliseconds(500);
        palClearLine(LINE_PIO0);
        chThdSleepMilliseconds(500);
    }

    while(!si446x_init(&brdcfg, &labradcfg)) {
        palSetLine(LINE_PIO0);
        chThdSleepMilliseconds(500);
        palClearLine(LINE_PIO0);
        chThdSleepMilliseconds(500);
    }
}

void lab01_labrador_run()
{
    labrador_thdp = chThdCreateStatic(
        lab01_labrador_thd_wa, sizeof(lab01_labrador_thd_wa),
        NORMALPRIO, lab01_labrador_thd, NULL);
}

void lab01_labrador_send(uint16_t msg_id, bool can_rtr,
                         uint8_t* data, uint8_t datalen)
{
    txbuf[0]  = (msg_id  >> 3) & 0x00FF;
    txbuf[1]  = (msg_id  << 5) & 0x00E0;
    txbuf[1] |= (can_rtr << 5) & 0x0010;
    txbuf[1] |= (datalen     ) & 0x000F;
    memcpy(&txbuf[2], data, datalen);
    if(labrador_thdp != NULL) {
        chMsgSend(labrador_thdp, (msg_t)txbuf);
    }
}
