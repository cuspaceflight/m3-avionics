#include "labrador.h"
#include "si446x.h"
#include "m3can.h"
#include "m3radio_status.h"
#include "m3radio_labrador.h"
#include "ch.h"
#include "chprintf.h"

#define TXCODE LDPC_CODE_N256_K128
#define RXCODE LDPC_CODE_N256_K128

static uint8_t labrador_wa[LDPC_SIZE(FAST, TXCODE, MP, RXCODE)];
static uint8_t txbuf[LDPC_PARAM_N(TXCODE)/8] = "Hello Labrador!";

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

/* Callback for the configuration dumping utility in the Si446x driver */
static void si446x_cfg_cb(uint8_t g, uint8_t p, uint8_t v)
{
    can_send_u8(CAN_MSG_ID_M3RADIO_SI4460_CFG, g, p, v, 0, 0, 0, 0, 0, 3);
}

THD_WORKING_AREA(m3radio_labrador_thd_wa, 1024);
THD_FUNCTION(m3radio_labrador_thd, arg) {
    (void)arg;

    m3status_set_init(M3RADIO_COMPONENT_LABRADOR);

    /* Initialise Labrador systems */
    labrador_err rv = labrador_init(&labcfg, &labradcfg,
                                    &labrador_radio_si446x);
    if(rv != LABRADOR_OK) {
        m3status_set_error(M3RADIO_COMPONENT_LABRADOR,
                           M3RADIO_ERROR_LABRADOR);

        while(true) {
            chThdSleepMilliseconds(100);
        }
    }

    /* Initialise the Si446x driver */
    if(!si446x_init(&brdcfg, &labradcfg)) {
        m3status_set_error(M3RADIO_COMPONENT_LABRADOR,
                           M3RADIO_ERROR_LABRADOR_SI4460);

        while(true) {
            chThdSleepMilliseconds(100);
        }
    }

    /* Dump the Si446x configuration to CAN for logging */
    si446x_dump_params(si446x_cfg_cb);

#define TX 1

#if TX
    int counter = 0;
#else
    uint8_t* rxbuf;
#endif

    while (true) {
        m3status_set_ok(M3RADIO_COMPONENT_LABRADOR);
#if TX
        chsnprintf((char*)txbuf, LDPC_PARAM_N(TXCODE)/8, "%015d", counter++);
        labrador_tx(txbuf);
        chThdSleepMilliseconds(200);
#else
        (void)txbuf;
        if(labrador_rx(&rxbuf) == LABRADOR_OK) {
            can_send((CAN_ID_M3RADIO | CAN_MSG_ID(60)), false, rxbuf, 8);
            can_send((CAN_ID_M3RADIO | CAN_MSG_ID(61)), false, rxbuf+8, 8);
        }
        chThdSleepMilliseconds(10);
#endif
    }
}

void m3radio_labrador_init()
{
    chThdCreateStatic(m3radio_labrador_thd_wa,
                      sizeof(m3radio_labrador_thd_wa),
                      NORMALPRIO,
                      m3radio_labrador_thd, NULL);
}
