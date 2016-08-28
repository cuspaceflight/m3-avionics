#include "si4460.h"
#include "ch.h"
#include "hal.h"
#include "m3radio_status.h"
#include "m3can.h"
#include "ezradiopro.h"

/* XXX Get correct CR1 settings for Si4460 */
static SPIDriver* si4460_spid;
static SPIConfig spi_cfg = {
    .end_cb = NULL,
    .ssport = 0,
    .sspad = 0,
    .cr1 = SPI_CR1_BR_2,
    /*.cr1 = SPI_CR1_BR_0 | SPI_CR1_BR_1 | SPI_CR1_CPOL | SPI_CR1_CPHA,*/
};

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

struct si4460_chip_status {
    uint8_t chip_pend, chip_status, cmd_err_status, cmd_err_cmd_id;
} __attribute__((packed));

struct si4460_int_status {
    uint8_t int_pend, int_status, ph_pend, ph_status, modem_pend, modem_status;
    uint8_t chip_pend, chip_status;
};

struct si4460_modem_status {
    uint8_t modem_pend, modem_status, curr_rssi, latch_rssi, ant1_rssi,
            ant2_rssi;
    uint16_t afc_freq_offset;
};

static bool si4460_configure(void);
static void si4460_send_command(uint8_t* txbuf, size_t txn, uint8_t* rxbuf, size_t rxn);

static void si4460_power_up(bool tcxo, uint32_t xo_freq);

static struct si4460_part_info si4460_part_info(void);
static struct si4460_func_info si4460_func_info(void);
static void si4460_set_property(uint8_t group, uint8_t number, uint8_t value);
static uint8_t si4460_get_property(uint8_t group, uint8_t number) __attribute__((used));
static void si4460_fifo_info(bool reset_rx, bool reset_tx, uint8_t* rx_fifo_count, uint8_t* tx_fifo_space) __attribute__((used));
static struct si4460_int_status si4460_get_int_status(uint8_t ch_clr_pend, uint8_t modem_clr_pend, uint8_t chip_clr_pend) __attribute__((used));
static void si4460_request_device_state(uint8_t* curr_state, uint8_t* current_channel) __attribute__((used));
static void si4460_change_state(uint8_t next_state1) __attribute__((used));
static void si4460_read_cmd_buf(uint8_t* buf, size_t n);
static void si4460_start_tx(uint8_t channel, uint8_t condition,
                            uint16_t tx_len, uint8_t tx_delay,
                            uint8_t num_repeats) __attribute__((used));
static void si4460_write_tx_fifo(uint8_t* data, size_t n) __attribute__((used));
static uint16_t si4460_packet_info(uint8_t field_number, uint16_t len, uint16_t len_diff) __attribute__((used));
static struct si4460_modem_status si4460_get_modem_status(uint8_t modem_clr_pend) __attribute__((used));
static void si4460_start_rx(uint8_t channel, uint8_t condition, uint16_t rx_len,
                            uint8_t next_state1, uint8_t next_state2, uint8_t next_state3) __attribute__((used));
static void si4460_read_rx_fifo(uint8_t* data, size_t n) __attribute__((used));
static struct si4460_chip_status si4460_get_chip_status(uint8_t chip_clr_pend);

static void si4460_send_command(uint8_t* txbuf, size_t txn,
                                uint8_t* rxbuf, size_t rxn) {
    spiSelect(si4460_spid);
    spiSend(si4460_spid, txn, txbuf);
    spiUnselect(si4460_spid);
    si4460_read_cmd_buf(rxbuf, rxn);
}

static void si4460_power_up(bool tcxo, uint32_t xo_freq)
{
    uint8_t buf[7] = {EZRP_POWER_UP, 0, tcxo,
                      xo_freq>>24, xo_freq>>16, xo_freq>>8, xo_freq};
    si4460_read_cmd_buf(NULL, 0);
    si4460_send_command(buf, sizeof(buf), NULL, 0);
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

static struct si4460_int_status si4460_get_int_status(uint8_t ch_clr_pend, uint8_t modem_clr_pend, uint8_t chip_clr_pend)
{
    uint8_t buf[4] = {EZRP_GET_INT_STATUS, ch_clr_pend, modem_clr_pend, chip_clr_pend};
    struct si4460_int_status int_status;
    si4460_send_command(buf, sizeof(buf), (uint8_t*)&int_status, sizeof(int_status));
    return int_status;
}

static void si4460_request_device_state(uint8_t* curr_state, uint8_t* current_channel)
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
        spiSelect(si4460_spid);
        spiSend(si4460_spid, 1, &cmd);
        spiReceive(si4460_spid, 1, &cts);
        if(cts == 0xFF) {
            break;
        }
        spiUnselect(si4460_spid);
    }

    /* Read actual command response if applicable */
    if(n > 0) {
        spiReceive(si4460_spid, n, buf);
    }

    spiUnselect(si4460_spid);
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
    spiSelect(si4460_spid);
    spiSend(si4460_spid, 1, &cmd);
    spiSend(si4460_spid, n, data);
    spiUnselect(si4460_spid);
}


static uint16_t si4460_packet_info(uint8_t field_number, uint16_t len, uint16_t len_diff)
{
    uint8_t buf[5] = {EZRP_PACKET_INFO, field_number, len>>8, len, len_diff};
    uint8_t rx[2];
    si4460_send_command(buf, sizeof(buf), rx, sizeof(rx));
    return (rx[0] << 8) | rx[1];
}

static struct si4460_modem_status si4460_get_modem_status(uint8_t modem_clr_pend)
{
    uint8_t buf[2] = {EZRP_GET_MODEM_STATUS, modem_clr_pend};
    struct si4460_modem_status modem_status;
    si4460_send_command(buf, sizeof(buf), (uint8_t*)&modem_status, sizeof(modem_status));
    modem_status.afc_freq_offset = (modem_status.afc_freq_offset >> 8) |
                                   (modem_status.afc_freq_offset << 8);
    return modem_status;
}

static void si4460_start_rx(uint8_t channel, uint8_t condition, uint16_t rx_len,
                            uint8_t next_state1, uint8_t next_state2, uint8_t next_state3)
{
    uint8_t buf[8] = {EZRP_START_RX, channel, condition, rx_len>>8, rx_len,
                      next_state1, next_state2, next_state3};
    si4460_send_command(buf, sizeof(buf), NULL, 0);
}

static void si4460_read_rx_fifo(uint8_t* data, size_t n)
{
    uint8_t cmd = EZRP_READ_RX_FIFO;
    spiSelect(si4460_spid);
    spiSend(si4460_spid, 1, &cmd);
    spiReceive(si4460_spid, n, data);
    spiUnselect(si4460_spid);
}

static struct si4460_chip_status si4460_get_chip_status(uint8_t chip_clr_pend)
{
    uint8_t buf[2] = {EZRP_GET_CHIP_STATUS, chip_clr_pend};
    struct si4460_chip_status chip_status;
    si4460_send_command(buf, sizeof(buf), (uint8_t*)&chip_status, sizeof(chip_status));
    return chip_status;
}


static bool si4460_configure() {
    struct si4460_chip_status chip_status;
    (void)chip_status;

    chip_status = si4460_get_chip_status(0);

    /* Power up with TCXO and 26MHz frequency */
    si4460_power_up(true, 26000000);

    chip_status = si4460_get_chip_status(0);

    struct si4460_part_info part_info = si4460_part_info();
    chip_status = si4460_get_chip_status(0);
    struct si4460_func_info func_info = si4460_func_info();
    chip_status = si4460_get_chip_status(0);

    (void)part_info;
    (void)func_info;


    /* Enable mysterious reserved bit and fast mode */
    si4460_set_property(EZRP_PROP_GLOBAL, EZRP_PROP_GLOBAL_CONFIG, (1<<6)|(1<<5));
    si4460_set_property(EZRP_PROP_GLOBAL, EZRP_PROP_GLOBAL_XO_TUNE, 0x00);
    chip_status = si4460_get_chip_status(0);

    /* Tune adjustment to zero for TCXO */
    si4460_set_property(EZRP_PROP_GLOBAL, EZRP_PROP_GLOBAL_XO_TUNE, 0x00);
    chip_status = si4460_get_chip_status(0);

    /* Set to 869.5MHz:
     * F = (INT + FRAC/2^19) * (XTAL/2)
     *   = (65 + 988081/2^19) * 13E6
     *   = 869.5MHz
     */
    si4460_set_property(EZRP_PROP_FREQ_CONTROL, EZRP_PROP_FREQ_CONTROL_INTE, 65);
    si4460_set_property(EZRP_PROP_FREQ_CONTROL, EZRP_PROP_FREQ_CONTROL_FRAC0, 0x0F);
    si4460_set_property(EZRP_PROP_FREQ_CONTROL, EZRP_PROP_FREQ_CONTROL_FRAC1, 0x13);
    si4460_set_property(EZRP_PROP_FREQ_CONTROL, EZRP_PROP_FREQ_CONTROL_FRAC2, 0xB1);

    /* 5 bytes preamble, require 20 bits RX */
    si4460_set_property(EZRP_PROP_PREAMBLE, EZRP_PROP_PREAMBLE_TX_LENGTH, 5);
    si4460_set_property(EZRP_PROP_PREAMBLE, EZRP_PROP_PREAMBLE_CONFIG_STD_1, 20);
    si4460_set_property(EZRP_PROP_PREAMBLE, EZRP_PROP_PREAMBLE_CONFIG, 0b00110001);

    /* 2 bytes default sync word */
    si4460_set_property(EZRP_PROP_SYNC, EZRP_PROP_SYNC_CONFIG, 0b00000001);

    /* Class E match PA, maximum output power, 25% duty cycle */
    si4460_set_property(EZRP_PROP_PA, EZRP_PROP_PA_MODE, 6<<2);
    si4460_set_property(EZRP_PROP_PA, EZRP_PROP_PA_PWR_LVL, 0x4F);
    si4460_set_property(EZRP_PROP_PA, EZRP_PROP_PA_BIAS_CLKDUTY, 3<<6);

    /* Packet config */
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_CONFIG1, 0x00);
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_LEN, 1<<3);
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_LEN_FIELD_SOURCE, 1);
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_LEN_ADJUST, 2);
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_TX_THRESHOLD, 20);
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_RX_THRESHOLD, 20);
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_FIELD_1_LENGTH0, 0);
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_FIELD_1_LENGTH1, 1);
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_FIELD_1_CONFIG, 0);
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_FIELD_1_CRC_CONFIG, 0);
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_FIELD_2_LENGTH0, 0);
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_FIELD_2_LENGTH1, 66);
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_FIELD_2_CONFIG, 0);
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_FIELD_2_CRC_CONFIG, 0);

    /* 2GFSK from FIFO */
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_MOD_TYPE, 3);
    /* No bit/freq inversions */
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_MAP_CONTROL, 0);
    /* Modem data rate at 10x baud rate */
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_DATA_RATE0, (uint8_t)(20000 >> 16));
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_DATA_RATE1, (uint8_t)(20000 >> 8));
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_DATA_RATE2, (uint8_t)(20000));
    /* NCO at TCXO freq */
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_TX_NCO_MODE0, (uint8_t)(26000000>>24));
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_TX_NCO_MODE1, (uint8_t)(26000000>>16));
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_TX_NCO_MODE2, (uint8_t)(26000000>>8));
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_TX_NCO_MODE3, (uint8_t)(26000000>>0));
    /* Freq dev at 24kHz */
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_FREQ_DEV0, 0x00);
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_FREQ_DEV0, 0x01);
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_FREQ_DEV0, 0xE4);
    /* PLL synth to FVCO/4 with SYSEL to high performance */
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_CLKGEN_BAND, 1<<3);

    return true;
}

static THD_WORKING_AREA(si4460_thd_wa, 512);
static THD_FUNCTION(si4460_thd, arg) {
    (void)arg;

    /* Reset the Si4460 */
    palSetLine(LINE_RADIO_SDN);
    chThdSleepMilliseconds(10);
    palClearLine(LINE_RADIO_SDN);
    chThdSleepMilliseconds(100);

    spiStart(si4460_spid, &spi_cfg);

    while(!si4460_configure()) {
        m3status_set_error(M3RADIO_COMPONENT_SI4460, M3RADIO_ERROR_SI4460_CFG);
        chThdSleepMilliseconds(1000);
    }

    while(true) {
        /*struct si4460_int_status int_status = si4460_get_int_status(0, 0, 0);*/
        struct si4460_part_info part_info = si4460_part_info();
        struct si4460_chip_status chip_status = si4460_get_chip_status(0);
        /*(void)int_status;*/
        (void)part_info;
        (void)chip_status;
        uint8_t state = EZRP_STATE_TX, channel;
        si4460_request_device_state(&state, &channel);
        uint8_t buf[27] = {22, '3', 'a', 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                           11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
                           22, 23, 24};
        si4460_write_tx_fifo(buf, 27);
        while(state == EZRP_STATE_TX) {
            si4460_request_device_state(&state, &channel);
        }
        si4460_start_tx(0, EZRP_STATE_READY << 4, 5, 0, 0);
        m3status_set_ok(M3RADIO_COMPONENT_SI4460);
        chThdSleepMilliseconds(100);
    }
}

void si4460_init(SPIDriver* spid, ioportid_t ssport, uint32_t sspad)
{
    m3status_set_init(M3RADIO_COMPONENT_SI4460);

    spi_cfg.ssport = ssport;
    spi_cfg.sspad = sspad;
    si4460_spid = spid;

    chThdCreateStatic(si4460_thd_wa, sizeof(si4460_thd_wa), NORMALPRIO,
                      si4460_thd, NULL);
}
