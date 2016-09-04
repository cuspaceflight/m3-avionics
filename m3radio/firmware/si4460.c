#include "si4460.h"
#include "ch.h"
#include "hal.h"
#include "m3radio_status.h"
#include "m3can.h"
#include "ezradiopro.h"

/* Temporary (I hope) bodge for sending data in. To be replaced with something
 * sensible like a mailbox+memorypool later.
 */
binary_semaphore_t si4460_tx_sem;
uint8_t si4460_tx_buf[60];

/* TODO: Can we run with BR0 and BR1 instead, a bit quicker? */
static SPIConfig spi_cfg = {
    .end_cb = NULL,
    .ssport = 0,
    .sspad = 0,
    .cr1 = SPI_CR1_BR_2,
};

/* The config we're passed in depends heavily on the board the radio is on,
 * e.g. what crystal, what SPI port, what CS pin, do we have shutdown control.
 * We'll keep it as this global because heck, everything else is terrible too.
 */
struct si4460_config* si4460_config;

/* These are "constants" for everything using our data mode,
 * but we'll probably want to tweak them in due course.
 */
static const uint32_t data_rate = 2000;
static const uint32_t deviation = 1000;

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

struct si4460_gpio_pin_cfg {
    uint8_t gpio0, gpio1, gpio2, gpio3, nirq, sdo, gen_config;
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

/******************************************************************************
 * Config for more sophisticated parameter-setting commands
 */
struct si4460_dec_cfg {
    uint8_t ndec0, ndec1, ndec2, ndec3;
    uint8_t ndec2gain;
    bool ndec2agc, chflt_lopw, droopflt, dwn2, dwn3, rxgainx2;
};

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
static void si4460_set_property(
    uint8_t group, uint8_t number, uint8_t value);
static uint8_t si4460_get_property(
    uint8_t group, uint8_t number);
static struct si4460_gpio_pin_cfg si4460_gpio_pin_cfg(
    struct si4460_gpio_pin_cfg gpio_pin_cfg);
static void si4460_fifo_info(
    bool reset_rx, bool reset_tx,
    uint8_t* rx_fifo_count, uint8_t* tx_fifo_space)
    __attribute__((used));
static struct si4460_int_status si4460_get_int_status(
    uint8_t ch_clr_pend, uint8_t modem_clr_pend, uint8_t chip_clr_pend)
    __attribute__((used));
static void si4460_request_device_state(
    uint8_t* curr_state, uint8_t* current_channel)
    __attribute__((used));
static void si4460_change_state(
    uint8_t next_state1)
    __attribute__((used));
static void si4460_read_cmd_buf(
    uint8_t* buf, size_t n);
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


/******************************************************************************
 * Our higher-level commands
 */
static bool si4460_configure(void);
static void si4460_set_freq(uint32_t f, uint32_t xo);
static void si4460_set_deviation(uint32_t d, uint32_t xo);
static void si4460_set_decimation(struct si4460_dec_cfg dec_cfg);
static void si4460_send_command(uint8_t* txbuf, size_t txn,
                                uint8_t* rxbuf, size_t rxn);
static void si4460_dump_config(void);
/*****************************************************************************/


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

static struct si4460_gpio_pin_cfg si4460_gpio_pin_cfg(
    struct si4460_gpio_pin_cfg gpio_pin_cfg)
{
    uint8_t buf[8] = {
        EZRP_GPIO_PIN_CFG,
        gpio_pin_cfg.gpio0, gpio_pin_cfg.gpio1,
        gpio_pin_cfg.gpio2, gpio_pin_cfg.gpio3,
        gpio_pin_cfg.nirq,  gpio_pin_cfg.sdo,
        gpio_pin_cfg.gen_config};
    struct si4460_gpio_pin_cfg pin_cfg;
    si4460_send_command(buf, sizeof(buf), (uint8_t*)&pin_cfg, sizeof(pin_cfg));
    return pin_cfg;
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
    uint32_t inte = (f / (xo / 2)) - 1;
    uint32_t frac = (((uint64_t)f << 19) / (xo / 2)) - ((inte + 1) << 19);

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
 * deviation (applies to TX only). */
static void si4460_set_deviation(uint32_t d, uint32_t xo) {
    uint32_t freq_dev = ((uint64_t)d<<20)/xo;
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_FREQ_DEV0,
        (uint8_t)(freq_dev>>16));
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_FREQ_DEV1,
        (uint8_t)(freq_dev>>8));
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_FREQ_DEV2,
        (uint8_t)(freq_dev>>0));
}

/* Compute and set the RX decimators */
static void si4460_set_decimation(struct si4460_dec_cfg dec_cfg) {
    uint8_t cfg0 = 0, cfg1 = 0, cfg2 = 0;

    if(dec_cfg.chflt_lopw) {
        cfg0 |= EZRP_MODEM_DECIMATION_CFG0_CHFLT_LOPW_LOWPOWER;
    } else {
        cfg0 |= EZRP_MODEM_DECIMATION_CFG0_CHFLT_LOPW_NORMAL;
    }

    if(dec_cfg.droopflt) {
        cfg0 |= EZRP_MODEM_DECIMATION_CFG0_DROOPFLTBYP_ENABLE;
    } else {
        cfg0 |= EZRP_MODEM_DECIMATION_CFG0_DROOPFLTBYP_BYPASS;
    }

    if(dec_cfg.dwn2) {
        cfg0 |= EZRP_MODEM_DECIMATION_CFG0_DWN2BYP_ENABLE;
    } else {
        cfg0 |= EZRP_MODEM_DECIMATION_CFG0_DWN2BYP_BYPASS;
    }

    if(dec_cfg.dwn3) {
        cfg0 |= EZRP_MODEM_DECIMATION_CFG0_DWN3BYP_ENABLE;
    } else {
        cfg0 |= EZRP_MODEM_DECIMATION_CFG0_DWN3BYP_BYPASS;
    }

    if(dec_cfg.rxgainx2) {
        cfg0 |= EZRP_MODEM_DECIMATION_CFG0_RXGAINX2_DOUBLE;
    } else {
        cfg0 |= EZRP_MODEM_DECIMATION_CFG0_RXGAINX2_NORMAL;
    }

    if(dec_cfg.ndec0 == 1) {
        cfg1 |= EZRP_MODEM_DECIMATION_CFG1_NDEC0_1;
    } else if(dec_cfg.ndec0 == 2) {
        cfg1 |= EZRP_MODEM_DECIMATION_CFG1_NDEC0_2;
    } else if(dec_cfg.ndec0 == 4) {
        cfg1 |= EZRP_MODEM_DECIMATION_CFG1_NDEC0_4;
    } else if(dec_cfg.ndec0 == 8) {
        cfg1 |= EZRP_MODEM_DECIMATION_CFG1_NDEC0_8;
    } else if(dec_cfg.ndec0 == 16) {
        cfg1 |= EZRP_MODEM_DECIMATION_CFG1_NDEC0_16;
    } else if(dec_cfg.ndec0 == 32) {
        cfg1 |= EZRP_MODEM_DECIMATION_CFG1_NDEC0_32;
    } else if(dec_cfg.ndec0 == 64) {
        cfg1 |= EZRP_MODEM_DECIMATION_CFG1_NDEC0_64;
    } else if(dec_cfg.ndec0 == 128) {
        cfg1 |= EZRP_MODEM_DECIMATION_CFG1_NDEC0_128;
    } else {
        m3status_set_error(M3RADIO_COMPONENT_SI4460, M3RADIO_ERROR_SI4460_CFG);
    }

    if(dec_cfg.ndec1 == 1) {
        cfg1 |= EZRP_MODEM_DECIMATION_CFG1_NDEC1_1;
    } else if(dec_cfg.ndec1 == 2) {
        cfg1 |= EZRP_MODEM_DECIMATION_CFG1_NDEC1_2;
    } else if(dec_cfg.ndec1 == 4) {
        cfg1 |= EZRP_MODEM_DECIMATION_CFG1_NDEC1_4;
    } else if(dec_cfg.ndec1 == 8) {
        cfg1 |= EZRP_MODEM_DECIMATION_CFG1_NDEC1_8;
    } else {
        m3status_set_error(M3RADIO_COMPONENT_SI4460, M3RADIO_ERROR_SI4460_CFG);
    }

    if(dec_cfg.ndec2 == 1) {
        cfg1 |= EZRP_MODEM_DECIMATION_CFG1_NDEC2_1;
    } else if(dec_cfg.ndec2 == 2) {
        cfg1 |= EZRP_MODEM_DECIMATION_CFG1_NDEC2_2;
    } else if(dec_cfg.ndec2 == 4) {
        cfg1 |= EZRP_MODEM_DECIMATION_CFG1_NDEC2_4;
    } else if(dec_cfg.ndec2 == 8) {
        cfg1 |= EZRP_MODEM_DECIMATION_CFG1_NDEC2_8;
    } else {
        m3status_set_error(M3RADIO_COMPONENT_SI4460, M3RADIO_ERROR_SI4460_CFG);
    }

    if(dec_cfg.ndec3 == 1) {
        cfg2 |= EZRP_MODEM_DECIMATION_CFG2_NDEC3_1;
    } else if(dec_cfg.ndec3 == 2) {
        cfg2 |= EZRP_MODEM_DECIMATION_CFG2_NDEC3_2;
    } else if(dec_cfg.ndec3 == 4) {
        cfg2 |= EZRP_MODEM_DECIMATION_CFG2_NDEC3_4;
    } else if(dec_cfg.ndec3 == 8) {
        cfg2 |= EZRP_MODEM_DECIMATION_CFG2_NDEC3_8;
    } else {
        m3status_set_error(M3RADIO_COMPONENT_SI4460, M3RADIO_ERROR_SI4460_CFG);
    }

    if(dec_cfg.ndec2gain == 0) {
        cfg2 |= EZRP_MODEM_DECIMATION_CFG2_NDEC2GAIN_GAIN0;
    } else if(dec_cfg.ndec2gain == 12) {
        cfg2 |= EZRP_MODEM_DECIMATION_CFG2_NDEC2GAIN_GAIN12;
    } else if(dec_cfg.ndec2gain == 24) {
        cfg2 |= EZRP_MODEM_DECIMATION_CFG2_NDEC2GAIN_GAIN24;
    } else {
        m3status_set_error(M3RADIO_COMPONENT_SI4460, M3RADIO_ERROR_SI4460_CFG);
    }

    if(dec_cfg.ndec2agc) {
        cfg2 |= EZRP_MODEM_DECIMATION_CFG2_NDEC2AGC_ENABLED;
    } else {
        cfg2 |= EZRP_MODEM_DECIMATION_CFG2_NDEC2AGC_DISABLED;
    }

    si4460_set_property(EZRP_PROP_MODEM,
                        EZRP_PROP_MODEM_DECIMATION_CFG0, cfg0);
    si4460_set_property(EZRP_PROP_MODEM,
                        EZRP_PROP_MODEM_DECIMATION_CFG1, cfg1);
    si4460_set_property(EZRP_PROP_MODEM,
                        EZRP_PROP_MODEM_DECIMATION_CFG2, cfg2);
}

/* Set the two BCR registers based on the clock, the decimation, the data rate.
 */
static void si4460_set_bcr(struct si4460_dec_cfg dec_cfg, uint32_t xo_freq,
                           uint16_t bcr_gain, uint8_t crfast, uint8_t crslow)
{
    /*
     * The BCR system runs off the RX sampling/filtering clock (except in OOK
     * mode where something odd happens with NDEC0), which is the XO frequency
     * divided down by all the decimators. We can work out how much division
     * there is and therefore what the RX clock freq is.
     * We then program the OSR register to NCO freq / data rate, even though
     * the documentation says this 8x the ratio between sample rate and data
     * rate, that appears to be either a lie or misleading.
     * The BCR NCO offset value is then added on to a 25-bit accumulator every
     * tick of the BCR clock, and is set so the accumulator overflows at the
     * data rate. In other words, offset=(2^25)/OSR.
     * Again the documentation implies this is really 64x the offset value and
     * there's some rubbish with fractional offsets, but it seems safe to
     * ignore that. Maybe think of the accumulator as having 6 fractional bits.
     * (PS: Why 25 bits? Who knows? Just fits the default values.)
     */
    float nco_freq = (float)xo_freq;
    if(dec_cfg.dwn2) nco_freq /= 2.0f;
    if(dec_cfg.dwn3) nco_freq /= 3.0f;
    nco_freq /= (float)dec_cfg.ndec3;
    nco_freq /= (float)dec_cfg.ndec2;
    nco_freq /= (float)dec_cfg.ndec1;
    nco_freq /= (float)dec_cfg.ndec0;
    uint32_t bcr_osr = (uint32_t)(nco_freq / (float)data_rate);
    uint32_t bcr_offset = (1<<25) / bcr_osr;

    if(bcr_osr >= (1<<12) || bcr_offset >= (1<<22)) {
        m3status_set_error(M3RADIO_COMPONENT_SI4460, M3RADIO_ERROR_SI4460_CFG);
    }

    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_BCR_OSR0,
        (uint8_t)(bcr_osr >> 8));
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_BCR_OSR1,
        (uint8_t)(bcr_osr >> 0));
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_BCR_NCO_OFFSET0,
        (uint8_t)(bcr_offset >> 16));
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_BCR_NCO_OFFSET1,
        (uint8_t)(bcr_offset >> 8));
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_BCR_NCO_OFFSET2,
        (uint8_t)(bcr_offset >> 0));

    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_BCR_GAIN0,
        (uint8_t)(bcr_gain >> 8));
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_BCR_GAIN1,
        (uint8_t)(bcr_gain >> 0));

    uint8_t osr0 = si4460_get_property(
        EZRP_PROP_MODEM, EZRP_PROP_MODEM_BCR_OSR0);
    uint8_t osr1 = si4460_get_property(
        EZRP_PROP_MODEM, EZRP_PROP_MODEM_BCR_OSR1);
    uint8_t offset0 = si4460_get_property(
        EZRP_PROP_MODEM, EZRP_PROP_MODEM_BCR_NCO_OFFSET0);
    uint8_t offset1 = si4460_get_property(
        EZRP_PROP_MODEM, EZRP_PROP_MODEM_BCR_NCO_OFFSET1);
    uint8_t offset2 = si4460_get_property(
        EZRP_PROP_MODEM, EZRP_PROP_MODEM_BCR_NCO_OFFSET2);
    (void)osr0; (void)osr1;
    (void)offset0; (void)offset1; (void)offset2;

    if(crfast >= (1<<3) || crslow >= (1<<3)) {
        m3status_set_error(M3RADIO_COMPONENT_SI4460, M3RADIO_ERROR_SI4460_CFG);
    }

    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_BCR_GEAR,
        EZRP_MODEM_BCR_GEAR_CRFAST(crfast)  |
        EZRP_MODEM_BCR_GEAR_CRSLOW(crslow));

    /* Recommended BCR settings here. Not sure by whom. Good luck. */
    /* TODO: Investigate better BCR settings. */
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_BCR_MISC1,
        EZRP_MODEM_BCR_MISC1_BCRFBBYP_ENABLED               |
        EZRP_MODEM_BCR_MISC1_SLICEFBBYP_ENABLED             |
        EZRP_MODEM_BCR_MISC1_RXNCOCOMP_DISABLED             |
        EZRP_MODEM_BCR_MISC1_RXCOMP_LAT_SAMP_PREAMBLE_END   |
        EZRP_MODEM_BCR_MISC1_CRGAINX2_NORMAL                |
        EZRP_MODEM_BCR_MISC1_DIS_MIDPT_ENABLED              |
        EZRP_MODEM_BCR_MISC1_ESC_MIDPT_ESCAPE_1CLK);
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_BCR_MISC0,
        EZRP_MODEM_BCR_MISC0_ADCWATCH_DISABLED              |
        EZRP_MODEM_BCR_MISC0_ADCRST_DISABLED                |
        EZRP_MODEM_BCR_MISC0_DISTOGG_NORMAL                 |
        EZRP_MODEM_BCR_MISC0_PH0SIZE_5);
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
     * no variable length fields.
     */
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_LEN,
        EZRP_PKT_LEN_INFINITE_LEN_NORMAL        |
        EZRP_PKT_LEN_ENDIAN_LITTLE              |
        EZRP_PKT_LEN_SIZE_ONE_BYTE              |
        EZRP_PKT_LEN_IN_FIFO_CUT_OUT            |
        EZRP_PKT_LEN_DST_FIELD(0));

    /* No need to adjust lengths */
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_LEN_ADJUST, 0);

    /* Set FIFO thresholds to 0 bytes. Don't really care. */
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_TX_THRESHOLD, 64);
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_RX_THRESHOLD, 64);

    /* Field 1 length to 60 bytes, i.e. the entire fixed size radio packet */
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_FIELD_1_LENGTH0, 0);
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_FIELD_1_LENGTH1, 60);

    /* No 4FSK, load fresh PN seed, enable whitening, disable Manchester */
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_FIELD_1_CONFIG,
        EZRP_PKT_FIELD_CONFIG_4FSK_DISABLE  |
        EZRP_PKT_FIELD_CONFIG_PN_START_LOAD |
        EZRP_PKT_FIELD_CONFIG_WHITEN_ENABLE |
        EZRP_PKT_FIELD_CONFIG_MANCH_DISABLE);

    /* Seed CRC at field 1, transmit it, check it, enable CRC. */
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_FIELD_1_CRC_CONFIG,
        EZRP_PKT_FIELD_CRC_CONFIG_CRC_START_CONTINUE    |
        EZRP_PKT_FIELD_CRC_CONFIG_ALT_CRC_START_LOAD    |
        EZRP_PKT_FIELD_CRC_CONFIG_SEND_CRC_ON           |
        EZRP_PKT_FIELD_CRC_CONFIG_SEND_ALT_CRC_OFF      |
        EZRP_PKT_FIELD_CRC_CONFIG_CHECK_CRC_ON          |
        EZRP_PKT_FIELD_CRC_CONFIG_CHECK_ALT_CRC_OFF     |
        EZRP_PKT_FIELD_CRC_CONFIG_CRC_ENABLE_ON         |
        EZRP_PKT_FIELD_CRC_CONFIG_ALT_CRC_ENABLE_OFF);

    /* Set field two to zero length to stop packet processing here. */
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_FIELD_2_LENGTH0, 0);
    si4460_set_property(EZRP_PROP_PKT, EZRP_PROP_PKT_FIELD_2_LENGTH1, 0);

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

    /* Modem data rate at 40x baud rate */
    /* TODO if data rate is higher we should change this automatically */
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_DATA_RATE0,
        (uint8_t)((40*data_rate) >> 16));
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_DATA_RATE1,
        (uint8_t)((40*data_rate) >> 8));
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_DATA_RATE2,
        (uint8_t)((40*data_rate) >> 0));

    /* NCO at XO freq. Applies to TX only. */
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_TX_NCO_MODE0,
                        EZRP_MODEM_TX_NCO_MODE_TXOSR_40X    |
                        (uint8_t)(si4460_config->xo_freq>>24));
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_TX_NCO_MODE1,
                        (uint8_t)(si4460_config->xo_freq>>16));
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_TX_NCO_MODE2,
                        (uint8_t)(si4460_config->xo_freq>>8));
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_TX_NCO_MODE3,
                        (uint8_t)(si4460_config->xo_freq>>0));

    /* Set centre frequency */
    si4460_set_freq(si4460_config->centre_freq, si4460_config->xo_freq);

    /* Freq dev */
    si4460_set_deviation(deviation, si4460_config->xo_freq);

    /* Operate on standard fixed IF */
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_IF_CONTROL,
        EZRP_MODEM_IF_CONTROL_ZEROIF_NORMAL     |
        EZRP_MODEM_IF_CONTROL_FIXIF_FIXED       |
        EZRP_MODEM_IF_CONTROL_ETSI_MODE_DISABLE);

    /* Set IF frequency. We want IF_FREQ_Hz = (xo_freq / 64),
     * and MODEM_IF_FREQ=(2^19 * 4 * IF_FREQ_Hz)/(2*xo_freq)
     *                  = 2^19 * 2^2 / 2^6 / 2^1 * (xo_freq/xo_freq)
     *                  = 2^14 = 16384, regardless of actual xo_freq
     */
    /* Actually don't set this for now, as we want it at the default
     * and it seems to break when we set it. ???
     * TODO ???
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_IF_FREQ0,
        (uint8_t)(16384>>16));
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_IF_FREQ1,
        (uint8_t)(16384>>8));
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_IF_FREQ2,
        (uint8_t)(16384>>0));
    */

    /* PLL synth to FVCO/4 with SYSEL to high performance */
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_CLKGEN_BAND,
        EZRP_MODEM_CLKGEN_BAND_FORCE_SY_RECAL_FORCE |
        EZRP_MODEM_CLKGEN_BAND_SY_SEL_HIGHPERF      |
        EZRP_MODEM_CLKGEN_BAND_BAND_FVCO_DIV_4);

    /* Select BCR phase source as phase computer. Don't know why. */
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_MDM_CTRL,
        EZRP_MODEM_MDM_CTRL_PH_SRC_SEL_PHASE_COMPUTER);

    /* Entirely undocumented with literally no idea what to do about it.
     * So..
     */
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_IFPKD_THRESHOLDS,
        0xE8);

    /* Set decimation to give suitable RX filter bandwidth and things,
     * then set the BCR registers accordingly.
     */
    /* TODO: BCR gain and gear gains need determining properly. */
    struct si4460_dec_cfg dec_cfg = {
        .ndec0 = 1,
        .ndec1 = 4,
        .ndec2 = 1,
        .ndec3 = 1,
        .ndec2gain = 12,
        .ndec2agc = true,
        .chflt_lopw = false,
        .droopflt = true,
        .dwn2 = false,
        .dwn3 = true,
        .rxgainx2 = false,
    };
    si4460_set_decimation(dec_cfg);
    si4460_set_bcr(dec_cfg, si4460_config->xo_freq, 0x0079, 0, 2);

    /* AFC gearing */
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_AFC_GEAR,
        EZRP_MODEM_AFC_GEAR_GEAR_SW_PREAMBLE    |
        EZRP_MODEM_AFC_GEAR_AFC_FAST(0)         |
        EZRP_MODEM_AFC_GEAR_AFC_SLOW(0));

    /* Enable AFC, default gains */
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_AFC_GAIN0,
        EZRP_MODEM_AFC_GAIN_ENAFC_ENABLE                |
        EZRP_MODEM_AFC_GAIN_AFCBD_DISABLE               |
        EZRP_MODEM_AFC_GAIN_AFC_GAIN_DIV_NO_REDUCTION   |
        EZRP_MODEM_AFC_GAIN_AFCGAIN_12_8(0x00));
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_AFC_GAIN1,
        EZRP_MODEM_AFC_GAIN_AFCGAIN_7_0(0x0A));

    /* AFC limit */
    /* TODO set automatically */
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_AFC_LIMITER0, 0x75);
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_AFC_LIMITER1, 0x35);

    /* AFC wait */
    /* TODO set automatically */
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_AFC_WAIT, 0x12);

    /* AFC over the whole packet, enable PLL correction */
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_AFC_MISC,
        EZRP_MODEM_AFC_MISC_ENAFCFRZ_AFC_FRZN_AFTER_GEAR_SW |
        EZRP_MODEM_AFC_MISC_ENFBPLL_ENABLE_AFC_COR_POLL     |
        EZRP_MODEM_AFC_MISC_EN2TB_EST_AFC_COR_2TB           |
        EZRP_MODEM_AFC_MISC_ENFZPMEND_NO_AFC_FRZN           |
        EZRP_MODEM_AFC_MISC_ENAFC_CLKSW_NO_CLK_SW           |
        EZRP_MODEM_AFC_MISC_NON_FRZEN_AFC_FRZN_CONSEC_BITS  |
        EZRP_MODEM_AFC_MISC_LARGE_FREQ_ERR_DISABLED);

    /* TODO: ??? */
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_AGC_RFPD_DECAY, 0xED);
    si4460_set_property(EZRP_PROP_MODEM, EZRP_PROP_MODEM_AGC_IFPD_DECAY, 0xED);

    /* RX filter coefficients
     * This is WDS filter 1, 132.27kHz bandwidth */
    si4460_set_property(EZRP_PROP_MODEM_CHFLT,
        EZRP_PROP_MODEM_CHFLT_RX1_CHFLT_COE13_7_0, 0xFF);
    si4460_set_property(EZRP_PROP_MODEM_CHFLT,
        EZRP_PROP_MODEM_CHFLT_RX1_CHFLT_COE12_7_0, 0xC4);
    si4460_set_property(EZRP_PROP_MODEM_CHFLT,
        EZRP_PROP_MODEM_CHFLT_RX1_CHFLT_COE11_7_0, 0x30);
    si4460_set_property(EZRP_PROP_MODEM_CHFLT,
        EZRP_PROP_MODEM_CHFLT_RX1_CHFLT_COE10_7_0, 0x7F);
    si4460_set_property(EZRP_PROP_MODEM_CHFLT,
        EZRP_PROP_MODEM_CHFLT_RX1_CHFLT_COE9_7_0, 0xF5);
    si4460_set_property(EZRP_PROP_MODEM_CHFLT,
        EZRP_PROP_MODEM_CHFLT_RX1_CHFLT_COE8_7_0, 0xB5);
    si4460_set_property(EZRP_PROP_MODEM_CHFLT,
        EZRP_PROP_MODEM_CHFLT_RX1_CHFLT_COE7_7_0, 0xB8);
    si4460_set_property(EZRP_PROP_MODEM_CHFLT,
        EZRP_PROP_MODEM_CHFLT_RX1_CHFLT_COE6_7_0, 0xDE);
    si4460_set_property(EZRP_PROP_MODEM_CHFLT,
        EZRP_PROP_MODEM_CHFLT_RX1_CHFLT_COE5_7_0, 0x05);
    si4460_set_property(EZRP_PROP_MODEM_CHFLT,
        EZRP_PROP_MODEM_CHFLT_RX1_CHFLT_COE4_7_0, 0x17);
    si4460_set_property(EZRP_PROP_MODEM_CHFLT,
        EZRP_PROP_MODEM_CHFLT_RX1_CHFLT_COE3_7_0, 0x16);
    si4460_set_property(EZRP_PROP_MODEM_CHFLT,
        EZRP_PROP_MODEM_CHFLT_RX1_CHFLT_COE2_7_0, 0x0C);
    si4460_set_property(EZRP_PROP_MODEM_CHFLT,
        EZRP_PROP_MODEM_CHFLT_RX1_CHFLT_COE1_7_0, 0x03);
    si4460_set_property(EZRP_PROP_MODEM_CHFLT,
        EZRP_PROP_MODEM_CHFLT_RX1_CHFLT_COE0_7_0, 0x00);
    si4460_set_property(EZRP_PROP_MODEM_CHFLT,
        EZRP_PROP_MODEM_CHFLT_RX1_CHFLT_COEM0, 0x15);
    si4460_set_property(EZRP_PROP_MODEM_CHFLT,
        EZRP_PROP_MODEM_CHFLT_RX1_CHFLT_COEM1, 0xFF);
    si4460_set_property(EZRP_PROP_MODEM_CHFLT,
        EZRP_PROP_MODEM_CHFLT_RX1_CHFLT_COEM2, 0x00);
    si4460_set_property(EZRP_PROP_MODEM_CHFLT,
        EZRP_PROP_MODEM_CHFLT_RX1_CHFLT_COEM3, 0x00);

    /* Set up GPIO1 to output raw RX data for debugging */
    struct si4460_gpio_pin_cfg pin_cfg_set = {
        .gpio0 = EZRP_GPIO_PIN_CFG_TRISTATE,
        .gpio1 = EZRP_GPIO_PIN_CFG_RX_RAW_DATA,
        .gpio2 = EZRP_GPIO_PIN_CFG_TRISTATE,
        .gpio3 = EZRP_GPIO_PIN_CFG_TRISTATE,
        .nirq  = EZRP_GPIO_PIN_CFG_NIRQ,
        .sdo   = EZRP_GPIO_PIN_CFG_SDO,
        .gen_config = EZRP_GPIO_PIN_CFG_GEN_CONFIG_DRV_STRENGTH_HIGH,
    };
    struct si4460_gpio_pin_cfg pin_cfg_get = si4460_gpio_pin_cfg(pin_cfg_set);
    (void)pin_cfg_get;

    /* Check chip status looks OK */
    chip_status = si4460_get_chip_status(0);
    if(chip_status.chip_status & EZRP_CHIP_STATUS_CMD_ERROR) {
        return false;
    }

    return true;
}

/* Dump the chip's entire config out over CAN */
static void si4460_dump_config(void) {
    /* {Group ID, maximum group property ID} */
    uint8_t groups[][2] = {
        {EZRP_PROP_GLOBAL, EZRP_PROP_GLOBAL_WUT_CAL},
        {EZRP_PROP_INT_CTL, EZRP_PROP_INT_CTL_CHIP_ENABLE},
        {EZRP_PROP_FRR_CTL, EZRP_PROP_FRR_CTL_D_MODE},
        {EZRP_PROP_PREAMBLE, EZRP_PROP_PREAMBLE_POSTAMBLE_PATTERN3},
        {EZRP_PROP_SYNC, EZRP_PROP_SYNC_CONFIG2},
        {EZRP_PROP_PKT, EZRP_PROP_PKT_CRC_SEED3},
        {EZRP_PROP_MODEM, EZRP_PROP_MODEM_DSA_MISC},
        {EZRP_PROP_MODEM_CHFLT, EZRP_PROP_MODEM_CHFLT_RX2_CHFLT_COEM3},
        {EZRP_PROP_PA, EZRP_PROP_PA_DIG_PWR_SEQ_CONFIG},
        {EZRP_PROP_SYNTH, EZRP_PROP_SYNTH_VCO_KVCAL},
        {EZRP_PROP_MATCH, EZRP_PROP_MATCH_CTRL_4},
        {EZRP_PROP_FREQ_CONTROL, EZRP_PROP_FREQ_CONTROL_VCOCNT_RX_ADJ},
        {EZRP_PROP_RX_HOP, EZRP_PROP_RX_HOP_TABLE_SIZE},
        {EZRP_PROP_PTI, EZRP_PROP_PTI_LOG_EN},
    };

    size_t i, prop;
    for(i=0; i<sizeof(groups)/sizeof(groups[0]); i++) {
        for(prop=0; prop<=groups[i][1]; prop++) {
            uint8_t group = groups[i][0];
            uint8_t val = si4460_get_property(group, prop);
            can_send_u8(CAN_MSG_ID_M3RADIO_SI4460_CFG,
                        group, prop, val, 0, 0, 0, 0, 0, 3);
        }
    }
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

    si4460_dump_config();

    uint8_t packet[60];
    (void)packet;

    si4460_start_rx(0, 0, 0, EZRP_STATE_RX, EZRP_STATE_RX, EZRP_STATE_RX);

    while(true) {
        /*uint8_t state = EZRP_STATE_TX, channel;*/
        /*struct si4460_modem_status modem_status = si4460_get_modem_status(0);*/
        /*(void)modem_status;*/
        /*while(state == EZRP_STATE_TX) {*/
            /*si4460_request_device_state(&state, &channel);*/
            /*(void)state;*/
            /*(void)channel;*/
        /*}*/
        /*si4460_write_tx_fifo(packet, 60);*/
        /*si4460_start_tx(0, EZRP_START_TX_TXCOMPLETE_STATE(EZRP_STATE_READY),*/
                        /*0, 0, 0);*/
        /*chThdSleepMilliseconds(1000);*/

        struct si4460_int_status int_status = si4460_get_int_status(0, 0, 0);
        if(int_status.ph_pend & EZRP_PH_STATUS_PACKET_RX) {
            si4460_read_rx_fifo(packet, 60);
        }
        chThdSleepMilliseconds(100);
        m3status_set_ok(M3RADIO_COMPONENT_SI4460);
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
