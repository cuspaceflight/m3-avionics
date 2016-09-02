#include "si4460.h"
#include "ch.h"
#include "hal.h"
#include "m3radio_status.h"
#include "m3can.h"
#include "ezradiopro.h"

binary_semaphore_t si4460_tx_sem;
uint8_t si4460_tx_buf[60];

/* TODO: Can we run with BR0 and BR1 instead, a bit quicker? */
static SPIConfig spi_cfg = {
    .end_cb = NULL,
    .ssport = 0,
    .sspad = 0,
    .cr1 = SPI_CR1_BR_2,
};

struct si4460_config* si4460_config;

/******************************************************************************
 * Store result of larger commands
 */
struct si4460_part_info {
    uint8_t chiprev;
    uint16_t part;
    uint8_t pbuild;
    uint16_t id;
    uint8_t customer;
    uint8_t romid;
} __attribute__((packed));

struct si4460_func_info {
    uint8_t revext, revbranch, revint;
    uint16_t patch;
    uint8_t func;
} __attribute__((packed));

struct si4460_int_status {
    uint8_t int_pend, int_status, ph_pend, ph_status, modem_pend, modem_status;
    uint8_t chip_pend, chip_status;
} __attribute__((packed));

struct si4460_modem_status {
    uint8_t modem_pend, modem_status, curr_rssi, latch_rssi;
    uint8_t ant1_rssi, ant2_rssi;
    uint16_t afc_freq_offset;
} __attribute__((packed));

struct si4460_chip_status {
    uint8_t chip_pend, chip_status, cmd_err_status, cmd_err_cmd_id;
} __attribute__((packed));

struct si4460_ph_status  {
    uint8_t ph_pend, ph_status;
} __attribute__((packed));
/*****************************************************************************/

/*****************************************************************************
 * BOOT COMMANDS
 */
static void si4460_power_up(bool tcxo, uint32_t xo_freq);
/*****************************************************************************/

/*****************************************************************************
 * COMMON COMMANDS
 */
static struct si4460_part_info si4460_part_info(void);
static struct si4460_func_info si4460_func_info(void)
    __attribute__((used));
static void si4460_set_property(uint8_t group, uint8_t number, uint8_t value);
static uint8_t si4460_get_property(uint8_t group, uint8_t number);
static void si4460_fifo_info(bool reset_rx, bool reset_tx,
                             uint8_t* rx_fifo_count, uint8_t* tx_fifo_space)
    __attribute__((used));
static struct si4460_int_status si4460_get_int_status(uint8_t ch_clr_pend,
                                                      uint8_t modem_clr_pend,
                                                      uint8_t chip_clr_pend)
    __attribute__((used));
static void si4460_request_device_state(uint8_t* curr_state,
                                        uint8_t* current_channel);
static void si4460_change_state(uint8_t next_state1)
    __attribute__((used));
static void si4460_read_cmd_buf(uint8_t* buf, size_t n);
/*****************************************************************************/

/*****************************************************************************
 * TX COMMANDS
 */
static void si4460_start_tx(uint8_t channel, uint8_t condition,
                            uint16_t tx_len, uint8_t tx_delay,
                            uint8_t num_repeats)
    __attribute__((used));
static void si4460_write_tx_fifo(uint8_t* data, size_t n)
    __attribute__((used));
/*****************************************************************************/


/*****************************************************************************
 * RX COMMANDS
 */
static uint16_t si4460_packet_info(uint8_t field_number,
                                   uint16_t len, uint16_t len_diff)
    __attribute__((used));
static struct si4460_modem_status si4460_get_modem_status(
    uint8_t modem_clr_pend)
    __attribute__((used));
static void si4460_start_rx(uint8_t channel, uint8_t condition,
                            uint16_t rx_len, uint8_t next_state1,
                            uint8_t next_state2, uint8_t next_state3)
    __attribute__((used));
static void si4460_read_rx_fifo(uint8_t* data, size_t n)
    __attribute__((used));
/*****************************************************************************/


/*****************************************************************************
 * ADVANCED COMMANDS
 */
static struct si4460_chip_status si4460_get_chip_status(uint8_t chip_clr_pend);
static struct si4460_ph_status si4460_get_ph_status(uint8_t ph_clr_pend)
    __attribute__((used));
/*****************************************************************************/


/* Our higher-level functions */
static bool si4460_configure(void);
static void si4460_set_freq(uint32_t f, uint32_t xo);
static void si4460_set_dev(uint32_t d, uint32_t xo);
static void si4460_send_command(uint8_t* txbuf, size_t txn,
                                uint8_t* rxbuf, size_t rxn);


static void si4460_power_up(bool tcxo, uint32_t xo_freq)
{
    uint8_t buf[7] = {EZRP_POWER_UP, 1, tcxo,
                      xo_freq>>24, xo_freq>>16, xo_freq>>8, xo_freq};
    si4460_read_cmd_buf(NULL, 0);
    si4460_send_command(buf, sizeof(buf), NULL, 0);
    si4460_read_cmd_buf(NULL, 0);
}

static struct si4460_part_info si4460_part_info(void)
{
    uint8_t cmd = EZRP_PART_INFO;
    struct si4460_part_info part_info;
    si4460_send_command(&cmd, 1, (uint8_t*)&part_info, sizeof(part_info));
    part_info.part = (part_info.part >> 8) | (part_info.part << 8);
    part_info.id = (part_info.id >> 8) | (part_info.id << 8);
    return part_info;
}

static struct si4460_func_info si4460_func_info()
{
    uint8_t cmd = EZRP_FUNC_INFO;
    struct si4460_func_info func_info;
    si4460_send_command(&cmd, 1, (uint8_t*)&func_info, sizeof(func_info));
    func_info.patch = (func_info.patch >> 8) | (func_info.patch << 8);
    return func_info;
}

static void si4460_set_property(uint8_t group, uint8_t number, uint8_t value)
{
    uint8_t buf[5] = {EZRP_SET_PROPERTY, group, 1, number, value};
    si4460_send_command(buf, sizeof(buf), NULL, 0);
}

static uint8_t si4460_get_property(uint8_t group, uint8_t number)
{
    uint8_t buf[4] = {EZRP_GET_PROPERTY, group, 1, number};
    uint8_t rx;
    si4460_send_command(buf, sizeof(buf), &rx, 1);
    return rx;
}

static void si4460_fifo_info(bool reset_rx, bool reset_tx,
                             uint8_t* rx_fifo_count, uint8_t* tx_fifo_space)
{
    uint8_t buf[2] = {EZRP_FIFO_INFO, (reset_rx<<1) | reset_tx};
    uint8_t rxbuf[2];
    si4460_send_command(buf, sizeof(buf), rxbuf, 2);
    *rx_fifo_count = rxbuf[0];
    *tx_fifo_space = rxbuf[1];
}

static struct si4460_int_status si4460_get_int_status(
    uint8_t ch_clr_pend, uint8_t modem_clr_pend, uint8_t chip_clr_pend)
{
    uint8_t buf[4] = {EZRP_GET_INT_STATUS, ch_clr_pend,
                      modem_clr_pend, chip_clr_pend};
    struct si4460_int_status int_status;
    si4460_send_command(buf, sizeof(buf),
                        (uint8_t*)&int_status, sizeof(int_status));
    return int_status;
}

static void si4460_request_device_state(
    uint8_t* curr_state, uint8_t* current_channel)
{
    uint8_t cmd = EZRP_REQUEST_DEVICE_STATE;
    uint8_t rxbuf[2];
    si4460_send_command(&cmd, 1, rxbuf, 2);
    *curr_state = rxbuf[0];
    *current_channel = rxbuf[1];
}

static void si4460_change_state(uint8_t next_state1)
{
    uint8_t buf[2] = {EZRP_CHANGE_STATE, next_state1};
    si4460_send_command(buf, sizeof(buf), NULL, 0);
}

static void si4460_read_cmd_buf(uint8_t* buf, size_t n)
{
    uint8_t cmd = EZRP_READ_CMD_BUFF;
    uint8_t cts = 0x00;

    /* Wait for CTS to be 0xFF */
    while(cts != 0xFF) {
        spiSelect(si4460_config->spid);
        spiSend(si4460_config->spid, 1, &cmd);
        spiReceive(si4460_config->spid, 1, &cts);
        if(cts == 0xFF) {
            break;
        }
        spiUnselect(si4460_config->spid);
    }

    /* Read actual command response if applicable */
    if(n > 0) {
        spiReceive(si4460_config->spid, n, buf);
    }

    spiUnselect(si4460_config->spid);
}


static void si4460_start_tx(uint8_t channel, uint8_t condition,
                            uint16_t tx_len, uint8_t tx_delay,
                            uint8_t num_repeats)
{
    uint8_t buf[7] = {EZRP_START_TX, channel, condition, tx_len>>8, tx_len,
                      tx_delay, num_repeats};
    si4460_send_command(buf, sizeof(buf), NULL, 0);
}

static void si4460_write_tx_fifo(uint8_t* data, size_t n)
{
    uint8_t cmd = EZRP_WRITE_TX_FIFO;
    spiSelect(si4460_config->spid);
    spiSend(si4460_config->spid, 1, &cmd);
    spiSend(si4460_config->spid, n, data);
    spiUnselect(si4460_config->spid);
}


static uint16_t si4460_packet_info(uint8_t field_number, uint16_t len,
                                   uint16_t len_diff)
{
    uint8_t buf[5] = {EZRP_PACKET_INFO, field_number, len>>8, len, len_diff};
    uint8_t rx[2];
    si4460_send_command(buf, sizeof(buf), rx, sizeof(rx));
    return (rx[0] << 8) | rx[1];
}

static struct si4460_modem_status si4460_get_modem_status(
    uint8_t modem_clr_pend)
{
    uint8_t buf[2] = {EZRP_GET_MODEM_STATUS, modem_clr_pend};
    struct si4460_modem_status modem_status;
    si4460_send_command(buf, sizeof(buf), (uint8_t*)&modem_status,
                        sizeof(modem_status));
    modem_status.afc_freq_offset = (modem_status.afc_freq_offset >> 8) |
                                   (modem_status.afc_freq_offset << 8);
    return modem_status;
}

static void si4460_start_rx(uint8_t channel, uint8_t condition,
                            uint16_t rx_len, uint8_t next_state1,
                            uint8_t next_state2, uint8_t next_state3)
{
    uint8_t buf[8] = {EZRP_START_RX, channel, condition, rx_len>>8, rx_len,
                      next_state1, next_state2, next_state3};
    si4460_send_command(buf, sizeof(buf), NULL, 0);
}

static void si4460_read_rx_fifo(uint8_t* data, size_t n)
{
    uint8_t cmd = EZRP_READ_RX_FIFO;
    spiSelect(si4460_config->spid);
    spiSend(si4460_config->spid, 1, &cmd);
    spiReceive(si4460_config->spid, n, data);
    spiUnselect(si4460_config->spid);
}

static struct si4460_chip_status si4460_get_chip_status(uint8_t chip_clr_pend)
{
    uint8_t buf[2] = {EZRP_GET_CHIP_STATUS, chip_clr_pend};
    struct si4460_chip_status chip_status;
    si4460_send_command(buf, sizeof(buf),
                        (uint8_t*)&chip_status, sizeof(chip_status));
    return chip_status;
}

static struct si4460_ph_status si4460_get_ph_status(uint8_t ph_clr_pend)
{
    uint8_t buf[2] = {EZRP_GET_PH_STATUS, ph_clr_pend};
    struct si4460_ph_status ph_status;
    si4460_send_command(buf, sizeof(buf),
                        (uint8_t*)&ph_status, sizeof(ph_status));
    return ph_status;
}

/* Send the txbuf to the SI4460, wait for CTS, read the specified length of
 * response.
 */
static void si4460_send_command(uint8_t* txbuf, size_t txn,
                                uint8_t* rxbuf, size_t rxn) {
    spiSelect(si4460_config->spid);
    spiSend(si4460_config->spid, txn, txbuf);
    spiUnselect(si4460_config->spid);
    si4460_read_cmd_buf(rxbuf, rxn);
}

/* Compute and set the required PLL configuration for the desired centre
 * frequency.
 * The set frequency is F = (1 + INTE + FRAC/2^19)*(XO_FREQ/2)
 */
static void si4460_set_freq(uint32_t f, uint32_t xo) {
    int inte = (f / (xo / 2)) - 1;
    int frac = ((f << 19) / (xo / 2)) - ((inte + 1) << 19);

    si4460_set_property(EZRP_PROP_FREQ_CONTROL, EZRP_PROP_FREQ_CONTROL_INTE,
        inte);
    si4460_set_property(EZRP_PROP_FREQ_CONTROL, EZRP_PROP_FREQ_CONTROL_FRAC0,
        (uint8_t)(frac>>16));
    si4460_set_property(EZRP_PROP_FREQ_CONTROL, EZRP_PROP_FREQ_CONTROL_FRAC1,
        (uint8_t)(frac>>8));
    si4460_set_property(EZRP_PROP_FREQ_CONTROL, EZRP_PROP_FREQ_CONTROL_FRAC2,
        (uint8_t)(frac>>0));
}

/* Compute and set the required modem configuration for the desired frequency
 * deviation. */
static void si4460_set_dev(uint32_t d, uint32_t xo) {
    uint32_t freq_dev = (d<<20)/xo;
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_FREQ_DEV0,
        (uint8_t)(freq_dev>>16));
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_FREQ_DEV1,
        (uint8_t)(freq_dev>>8));
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_FREQ_DEV2,
        (uint8_t)(freq_dev>>0));
}

static bool si4460_configure() {
    struct si4460_chip_status chip_status;
    struct si4460_part_info part_info;

    /* Check we have a 4460 as expected */
    part_info = si4460_part_info();
    if(part_info.part != 0x4460) {
        return false;
    }

    /* Power up with configured xo/tcxo and xo frequency */
    si4460_power_up(si4460_config->tcxo, si4460_config->xo_freq);

    /* Enable mysterious reserved bit and fast mode */
    uint8_t global_config_prop = (
        EZRP_GLOBAL_CONFIG_RESERVED                 |
        EZRP_GLOBAL_CONFIG_SEQUENCER_MODE_FAST      |
        EZRP_GLOBAL_CONFIG_FIFO_MODE_SPLIT_FIFO     |
        EZRP_GLOBAL_CONFIG_PROTOCOL_GENERIC         |
        EZRP_GLOBAL_CONFIG_POWER_MODE_HIGH_PERF
    );
    si4460_set_property(EZRP_PROP_GLOBAL, EZRP_PROP_GLOBAL_CONFIG,
        global_config_prop);

    /* Read back that parameter to check we're writing OK */
    uint8_t readback_prop = si4460_get_property(
        EZRP_PROP_GLOBAL, EZRP_PROP_GLOBAL_CONFIG);
    if(readback_prop != global_config_prop) {
        return false;
    }

    if(si4460_config->tcxo) {
        /* Tune adjustment to zero for TCXO */
        si4460_set_property(EZRP_PROP_GLOBAL, EZRP_PROP_GLOBAL_XO_TUNE, 0x00);
    }

    /* Set centre frequency */
    si4460_set_freq(si4460_config->centre_freq, si4460_config->xo_freq);

    /* Preamble config *******************************************************/
    /* 5 bytes preamble, require 20 bits RX, wait a long time before deciding
     * we haven't seen a preamble and timing out. */
    si4460_set_property(EZRP_PROP_PREAMBLE, EZRP_PROP_PREAMBLE_TX_LENGTH, 5);
    si4460_set_property(EZRP_PROP_PREAMBLE, EZRP_PROP_PREAMBLE_CONFIG_STD_1,
        EZRP_PREAMBLE_CONFIG_STD_1_SKIP_SYNC_TIMEOUT_DISABLE |
        EZRP_PREAMBLE_CONFIG_STD_1_RX_THRESH(20));
    si4460_set_property(EZRP_PROP_PREAMBLE, EZRP_PROP_PREAMBLE_CONFIG_STD_2,
        EZRP_PREAMBLE_CONFIG_STD_2_RX_PREAMBLE_TIMEOUT_EXTEND(0xF));
    si4460_set_property(EZRP_PROP_PREAMBLE, EZRP_PROP_PREAMBLE_CONFIG,
        EZRP_PREAMBLE_CONFIG_RX_PREAM_SRC_STANDARD_PREAM    |
        EZRP_PREAMBLE_CONFIG_PREAM_FIRST_1_OR_0_FIRST_1     |
        EZRP_PREAMBLE_CONFIG_LENGTH_CONFIG_BYTE             |
        EZRP_PREAMBLE_CONFIG_MAN_CONST_NO_CON               |
        EZRP_PREAMBLE_CONFIG_MAN_ENABLE_NO_MAN              |
        EZRP_PREAMBLE_CONFIG_STANDARD_PREAM_PRE_1010);

    /* Sync word config ******************************************************/
    /* TX sync word, don't allow any RX errors, not 4FSK, no Manchester coding,
     * use a two-byte sync word 0x2D 0xD4. */
    si4460_set_property(EZRP_PROP_SYNC, EZRP_PROP_SYNC_CONFIG,
        EZRP_SYNC_CONFIG_SKIP_TX_SYNC_XMIT      |
        EZRP_SYNC_CONFIG_RX_ERRORS(0)           |
        EZRP_SYNC_CONFIG_4FSK_DISABLED          |
        EZRP_SYNC_CONFIG_MANCH_DISABLED         |
        EZRP_SYNC_CONFIG_LENGTH_LEN_2_BYTES);
    si4460_set_property(EZRP_PROP_SYNC, EZRP_PROP_SYNC_CONFIG2,
        EZRP_SYNC_CONFIG2_SYNC_ERROR_ONLY_BEGIN_SYNC_ERROR_RAND     |
        EZRP_SYNC_CONFIG2_LENGTH_SUB_SUB_0);
    si4460_set_property(EZRP_PROP_SYNC, EZRP_PROP_SYNC_BITS0, 0x2D);
    si4460_set_property(EZRP_PROP_SYNC, EZRP_PROP_SYNC_BITS1, 0xD4);

    /* PA config *************************************************************/
    /* Class E match PA, maximum output power, 25% duty cycle */
    si4460_set_property(EZRP_PROP_PA, EZRP_PROP_PA_MODE,
        EZRP_PA_MODE_EXT_PA_RAMP_DISABLE    |
        EZRP_PA_MODE_DIG_PWR_SEQ_DISABLE    |
        EZRP_PA_MODE_PA_SEL_LP              |
        EZRP_PA_MODE_PA_MODE_CLE);
    si4460_set_property(EZRP_PROP_PA, EZRP_PROP_PA_PWR_LVL,
        EZRP_PA_PWR_LVL_MAXIMUM);
    si4460_set_property(EZRP_PROP_PA, EZRP_PROP_PA_BIAS_CLKDUTY,
        EZRP_PA_BIAS_CLKDUTY_CLK_DUTY_SINGLE_25);

    /* Packet config *********************************************************/
    /* CRC seed 0xFF, no Alt CRC, main CRC set to CCITT_16 */
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_CRC_CONFIG,
        EZRP_PKT_CRC_CONFIG_CRC_SEED_CRC_SEED_0         |
        EZRP_PKT_CRC_CONFIG_ALT_CRC_POLYNOMIAL_NO_CRC   |
        EZRP_PKT_CRC_CONFIG_CRC_POLYNOMINAL_CCITT_16);

    /* Whitening polynominal 0x0108, seed 0xFFFF, whitening enabled,
     * packet CRC enabled, whiten forward and using bit 0 of the LFSR */
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_WHT_POLY0, 0x01);
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_WHT_POLY1, 0x08);
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_WHT_SEED0, 0xFF);
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_WHT_SEED1, 0xFF);
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_WHT_BIT_NUM,
        EZRP_PKT_WHT_BIT_NUM_SW_WHT_CTRL_ENABLE     |
        EZRP_PKT_WHT_BIT_NUM_SW_CRC_CTRL_ENABLE     |
        EZRP_PKT_WHT_BIT_NUM_WHT_BIT_NUM(0));

    /* Share field settings between RX and TX,
     * enable packet handler in RX, disable 4FSK,
     * default Manchester polarity (unused), don't invert the CRC,
     * CRC is LSB first, data bytes transmitted MSb first
     */
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_CONFIG1,
        EZRP_PKT_CONFIG1_PH_FIELD_SPLIT_FIELD_SHARED    |
        EZRP_PKT_CONFIG1_PH_RX_DISABLE_RX_ENABLED       |
        EZRP_PKT_CONFIG1_4FSK_EN_DISABLE                |
        EZRP_PKT_CONFIG1_MANCH_POL_PATTERN_10           |
        EZRP_PKT_CONFIG1_CRC_INVERT_NO_INVERT           |
        EZRP_PKT_CONFIG1_CRC_ENDIAN_LSBYTE_FIRST        |
        EZRP_PKT_CONFIG1_BIT_ORDER_MSBIT_FIRST);

    /* CRC transmitted MSb first, CRC not padded,
     * default alt CRC seed (unused), disable 3of6 encoding
     */
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_CONFIG2,
        EZRP_PKT_CONFIG2_CRC_BIT_ENDIAN_MSBIT_FIRST     |
        EZRP_PKT_CONFIG2_CRC_PADDING_NO_PADDING         |
        EZRP_PKT_CONFIG2_ALT_CRC_SEED_ALT_CRC_SEED_0    |
        EZRP_PKT_CONFIG2_EN_3_OF_6_DISABLED);

    /* Non-infinite packet lengths, little endian packet length field,
     * one byte packet length field, packet length not placed in rx FIFO,
     * field 2 is the variable length field
     */
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_LEN,
        EZRP_PKT_LEN_INFINITE_LEN_NORMAL        |
        EZRP_PKT_LEN_ENDIAN_LITTLE              |
        EZRP_PKT_LEN_SIZE_ONE_BYTE              |
        EZRP_PKT_LEN_IN_FIFO_CUT_OUT            |
        EZRP_PKT_LEN_DST_FIELD(2));

    /* Use field 1 as containing the length byte */
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_LEN_FIELD_SOURCE, 1);

    /* We'll send lengths that just include data, not length or CRC */
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_LEN_ADJUST, 0);

    /* Set FIFO thresholds to 0 bytes. Don't really care. */
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_TX_THRESHOLD, 0);
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_RX_THRESHOLD, 0);

    /* Field 1 length to 1 byte, i.e. the packet length byte */
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_FIELD_1_LENGTH0, 0);
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_FIELD_1_LENGTH1, 1);

    /* No 4FSK, load fresh PN seed, enable whitening, disable Manchester */
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_FIELD_1_CONFIG,
        EZRP_PKT_FIELD_CONFIG_4FSK_DISABLE  |
        EZRP_PKT_FIELD_CONFIG_PN_START_LOAD |
        EZRP_PKT_FIELD_CONFIG_WHITEN_ENABLE |
        EZRP_PKT_FIELD_CONFIG_MANCH_DISABLE);

    /* Seed CRC at field 1, don't transmit it yet, don't check it yet,
     * do enable CRC for this field. */
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_FIELD_1_CRC_CONFIG,
        EZRP_PKT_FIELD_CRC_CONFIG_CRC_START_CONTINUE    |
        EZRP_PKT_FIELD_CRC_CONFIG_ALT_CRC_START_LOAD    |
        EZRP_PKT_FIELD_CRC_CONFIG_SEND_CRC_OFF          |
        EZRP_PKT_FIELD_CRC_CONFIG_SEND_ALT_CRC_OFF      |
        EZRP_PKT_FIELD_CRC_CONFIG_CHECK_CRC_OFF         |
        EZRP_PKT_FIELD_CRC_CONFIG_CHECK_ALT_CRC_OFF     |
        EZRP_PKT_FIELD_CRC_CONFIG_CRC_ENABLE_ON         |
        EZRP_PKT_FIELD_CRC_CONFIG_ALT_CRC_ENABLE_OFF);

    /* Set field two to maximum expected length of the field, which is 60
     * = 5 12-byte full-size CAN packets
     * (and fits inside the 64b RX FIFO)
     */
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_FIELD_2_LENGTH0, 0);
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_FIELD_2_LENGTH1, 60);

    /* No 4FSK, enable whitening, disable Manchester */
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_FIELD_2_CONFIG,
        EZRP_PKT_FIELD_CONFIG_4FSK_DISABLE  |
        EZRP_PKT_FIELD_CONFIG_WHITEN_ENABLE |
        EZRP_PKT_FIELD_CONFIG_MANCH_DISABLE);

    /* Send CRC, check CRC, enable CRC */
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_FIELD_2_CRC_CONFIG,
        EZRP_PKT_FIELD_CRC_CONFIG_SEND_CRC_ON           |
        EZRP_PKT_FIELD_CRC_CONFIG_SEND_ALT_CRC_OFF      |
        EZRP_PKT_FIELD_CRC_CONFIG_CHECK_CRC_ON          |
        EZRP_PKT_FIELD_CRC_CONFIG_CHECK_ALT_CRC_OFF     |
        EZRP_PKT_FIELD_CRC_CONFIG_CRC_ENABLE_ON         |
        EZRP_PKT_FIELD_CRC_CONFIG_ALT_CRC_ENABLE_OFF);

    /* Set field three length to zero to symbol the end of the payload */
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_FIELD_3_LENGTH0, 0);
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_FIELD_3_LENGTH1, 0);

    /* Modem config *********************************************************/
    /* 2GFSK from FIFO */
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_MOD_TYPE,
        EZRP_MODEM_MOD_TYPE_TX_DIRECT_MODE_TYPE_SYNC    |
        EZRP_MODEM_MOD_TYPE_MOD_SOURCE_PACKET           |
        EZRP_MODEM_MOD_TYPE_MOD_TYPE_2GFSK);

    /* No bit/freq inversions */
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_MAP_CONTROL,
        EZRP_MODEM_MAP_CONTROL_ENMANCH_NOADJUST     |
        EZRP_MODEM_MAP_CONTROL_ENINV_RXBIT_NOINVERT |
        EZRP_MODEM_MAP_CONTROL_ENINV_TXBIT_NOINVERT |
        EZRP_MODEM_MAP_CONTROL_ENINV_TXBIT_NOINVERT);

    /* Modem data rate at 10x baud rate */
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_DATA_RATE0,
        (uint8_t)((10*si4460_config->data_rate) >> 16));
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_DATA_RATE1,
        (uint8_t)((10*si4460_config->data_rate) >> 8));
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_DATA_RATE2,
        (uint8_t)((10*si4460_config->data_rate) >> 0));

    /* NCO at XO freq */
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_TX_NCO_MODE0,
                        (uint8_t)(si4460_config->xo_freq>>24));
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_TX_NCO_MODE1,
                        (uint8_t)(si4460_config->xo_freq>>16));
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_TX_NCO_MODE2,
                        (uint8_t)(si4460_config->xo_freq>>8));
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_TX_NCO_MODE3,
                        (uint8_t)(si4460_config->xo_freq>>0));

    /* Freq dev */
    si4460_set_dev(si4460_config->deviation, si4460_config->xo_freq);

    /* PLL synth to FVCO/4 with SYSEL to high performance */
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_CLKGEN_BAND,
        EZRP_MODEM_CLKGEN_BAND_FORCE_SY_RECAL_FORCE |
        EZRP_MODEM_CLKGEN_BAND_SY_SEL_HIGHPERF      |
        EZRP_MODEM_CLKGEN_BAND_BAND_FVCO_DIV_4);

    /* AFC gearing */
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_AFC_GEAR,
        EZRP_MODEM_AFC_GEAR_GEAR_SW_PREAMBLE    |
        EZRP_MODEM_AFC_GEAR_AFC_FAST(2)         |
        EZRP_MODEM_AFC_GEAR_AFC_SLOW(2));

    /* Enable AFC, default gains */
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_AFC_GAIN0,
        EZRP_MODEM_AFC_GAIN_ENAFC_ENABLE                |
        EZRP_MODEM_AFC_GAIN_AFCBD_DISABLE               |
        EZRP_MODEM_AFC_GAIN_AFC_GAIN_DIV_NO_REDUCTION   |
        EZRP_MODEM_AFC_GAIN_AFCGAIN_12_8(0x3));
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_AFC_GAIN1,
        EZRP_MODEM_AFC_GAIN_AFCGAIN_7_0(0x69));

    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_AFC_MISC,
        EZRP_MODEM_AFC_MISC_ENAFCFRZ_AFC_PKT                |
        EZRP_MODEM_AFC_MISC_ENFBPLL_ENABLE_AFC_COR_POLL     |
        EZRP_MODEM_AFC_MISC_EN2TB_EST_AFC_COR_2TB           |
        EZRP_MODEM_AFC_MISC_ENFZPMEND_NO_AFC_FRZN           |
        EZRP_MODEM_AFC_MISC_ENAFC_CLKSW_NO_CLK_SW           |
        EZRP_MODEM_AFC_MISC_NON_FRZEN_AFC_FRZN_CONSEC_BITS  |
        EZRP_MODEM_AFC_MISC_LARGE_FREQ_ERR_DISABLED);

    /* Check chip status looks OK */
    chip_status = si4460_get_chip_status(0);
    if(chip_status.chip_status & EZRP_CHIP_STATUS_CMD_ERROR) {
        return false;
    }

    return true;
}

static THD_WORKING_AREA(si4460_thd_wa, 512);
static THD_FUNCTION(si4460_thd, arg) {
    (void)arg;

    /* Reset the Si4460 */
    if(si4460_config->sdn) {
        palSetLine(si4460_config->sdnline);
        chThdSleepMilliseconds(10);
        palClearLine(si4460_config->sdnline);
        chThdSleepMilliseconds(100);
    }

    spiStart(si4460_config->spid, &spi_cfg);

    /* Configure the radio's many, many parameters */
    while(!si4460_configure()) {
        m3status_set_error(M3RADIO_COMPONENT_SI4460, M3RADIO_ERROR_SI4460_CFG);
        chThdSleepMilliseconds(1000);
    }

    uint8_t packet[10] = {9, 0, 1, 2, 3, 4, 5, 6, 7, 8};

    while(true) {
        uint8_t state = EZRP_STATE_TX, channel;
        struct si4460_int_status int_status = si4460_get_int_status(0, 0, 0);
        (void)int_status;
        /*si4460_start_rx(0, 0, 10, 8, 8, 8);*/
        while(state == EZRP_STATE_TX) {
            si4460_request_device_state(&state, &channel);
        }
        si4460_write_tx_fifo(packet, 10);
        si4460_start_tx(0, EZRP_START_TX_TXCOMPLETE_STATE(EZRP_STATE_READY),
                        0, 0, 0);
        m3status_set_ok(M3RADIO_COMPONENT_SI4460);
        chThdSleepMilliseconds(500);
    }
}

void si4460_init(struct si4460_config* config) {
    m3status_set_init(M3RADIO_COMPONENT_SI4460);

    chBSemObjectInit(&si4460_tx_sem, false);

    spi_cfg.ssport = config->ssport;
    spi_cfg.sspad = config->sspad;
    si4460_config = config;

    chThdCreateStatic(si4460_thd_wa, sizeof(si4460_thd_wa), NORMALPRIO,
                      si4460_thd, NULL);
}
