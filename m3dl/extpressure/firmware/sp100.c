#include "sp100.h"

#define SP100_CMD_MEASURE_A     0x85
#define SP100_CMD_RCAD          0x80
#define SP100_CMD_RRADH         0x81
#define SP100_CMD_RRADL         0x83
#define SP100_CMD_MEASURE_P     0x31
#define SP100_CMD_RCPDH         0x2C
#define SP100_CMD_RCPDL         0x2D
#define SP100_CMD_RRPDH         0x08
#define SP100_CMD_RRPDL         0x04
#define SP100_CMD_READIDH       0x3D
#define SP100_CMD_READIDM       0x38
#define SP100_CMD_READIDL       0x34
#define SP100_CMD_MEASURE_B     0x25
#define SP100_CMD_RCBD          0x29
#define SP100_CMD_MEASURE_T     0xAD
#define SP100_CMD_RCTMP         0xB0
#define SP100_CMD_RSR           0xB5
#define SP100_CMD_READ          0xB8
#define SP100_CMD_OTP_PWR       0xF6
#define SP100_CMD_RADDR         0xC1
#define SP100_CMD_STANDBY       0x01
#define SP100_CMD_LDOTP         0xBC
#define SP100_CMD_SETWK         0x2E
#define SP100_CMD_GETWK         0x2F

static SPIDriver* sp100_spid;
static ioline_t* sp100_ss;
static size_t sp100_nss;
static struct SP100* sp100_out;

static void sp100_spi_start(int n);
static void sp100_spi_stop(void);
static void sp100_reset(int n);
static void sp100_acceleration_measure(int n);
static void sp100_acceleration_read(int n);
static void sp100_temperature_measure(int n);
static void sp100_temperature_read(int n);
static void sp100_battery_measure(int n);
static void sp100_battery_read(int n);
static void sp100_pressure_measure(int n);
static void sp100_pressure_read(int n);
static void sp100_id(int n);
static uint8_t sp100_status(int n);
static uint8_t sp100_read_byte(int n, uint8_t cmd);
static void sp100_send_cmd(int n, uint8_t cmd);

static SPIConfig spi_cfg = {
    .end_cb = NULL,
    .ssport = 0,
    .sspad  = 0,
    .cr1    = SPI_CR1_BR_1 | SPI_CR1_BR_2 | SPI_CR1_CPHA | SPI_CR1_LSBFIRST,
};

static void sp100_spi_start(int n) {
    spi_cfg.ssport = PAL_PORT(sp100_ss[n]);
    spi_cfg.sspad  = PAL_PAD(sp100_ss[n]);
    spiStart(sp100_spid, &spi_cfg);
}

static void sp100_spi_stop() {
    spiStop(sp100_spid);
}

static void sp100_reset(int n) {
    sp100_send_cmd(n, SP100_CMD_LDOTP);
}

static void sp100_acceleration_measure(int n) {
    sp100_send_cmd(n, SP100_CMD_MEASURE_A);
}

static void sp100_acceleration_read(int n) {
    sp100_out[n].acceleration = sp100_read_byte(n, SP100_CMD_RCAD);
}

static void sp100_temperature_measure(int n) {
    sp100_send_cmd(n, SP100_CMD_MEASURE_T);
}

static void sp100_temperature_read(int n) {
    sp100_out[n].temperature = sp100_read_byte(n, SP100_CMD_RCTMP);
}

static void sp100_battery_measure(int n) {
    sp100_send_cmd(n, SP100_CMD_MEASURE_B);
}

static void sp100_battery_read(int n) {
    sp100_out[n].battery = sp100_read_byte(n, SP100_CMD_RCBD);
}

static void sp100_pressure_measure(int n) {
    sp100_send_cmd(n, SP100_CMD_MEASURE_P);
}

static void sp100_pressure_read(int n) {
    uint16_t pressure = 0;
    pressure |= sp100_read_byte(n, SP100_CMD_RCPDH) << 8;
    pressure |= sp100_read_byte(n, SP100_CMD_RCPDL);
    sp100_out[n].pressure = pressure;
}

static void sp100_id(int n) {
    uint32_t id = 0;
    id |= sp100_read_byte(n, SP100_CMD_READIDH) << 16;
    id |= sp100_read_byte(n, SP100_CMD_READIDM) << 8;
    id |= sp100_read_byte(n, SP100_CMD_READIDL);
    sp100_out[n].id = id;
}

static uint8_t sp100_status(int n) {
    return sp100_read_byte(n, SP100_CMD_RSR);
}

static uint8_t sp100_read_byte(int n, uint8_t cmd) {
    uint8_t rxb[1], txb[1];
    txb[0] = cmd;

    sp100_spi_start(n);

    spiSelect(sp100_spid);
    spiSend(sp100_spid, 1, txb);
    spiUnselect(sp100_spid);
    chThdSleepMicroseconds(60);

    spiSelect(sp100_spid);
    spiReceive(sp100_spid, 1, rxb);
    spiUnselect(sp100_spid);
    chThdSleepMicroseconds(60);

    sp100_spi_stop();

    return rxb[0];
}

static void sp100_send_cmd(int n, uint8_t cmd) {
    uint8_t txb[1];
    txb[0] = cmd;

    sp100_spi_start(n);

    spiSelect(sp100_spid);
    spiSend(sp100_spid, 1, txb);
    spiUnselect(sp100_spid);
    chThdSleepMicroseconds(60);

    sp100_spi_stop();
}

static THD_WORKING_AREA(sp100_thd_wa, 256);
static THD_FUNCTION(sp100_thd, arg) {
    (void)arg;
    size_t i;

    /* Wait for SP100 10ms POR */
    chThdSleepMilliseconds(12);

    /* Force reset of all pressure sensors */
    for(i=0; i<sp100_nss; i++) {
        sp100_reset(i);
    }

    /* Wait for reset to be done */
    chThdSleepMilliseconds(10);

    /* Read all IDs */
    for(i=0; i<sp100_nss; i++) {
        sp100_id(i);
    }

    /* Go around the loop reading sensors */
    while(true) {
        for(i=0; i<sp100_nss; i++) {
            volatile uint8_t status;

            sp100_id(i);
            chThdSleepMilliseconds(1);

            sp100_battery_measure(i);
            chThdSleepMilliseconds(20);
            status = sp100_status(i);
            sp100_battery_read(i);
            chThdSleepMilliseconds(1);

            sp100_temperature_measure(i);
            chThdSleepMilliseconds(20);
            status = sp100_status(i);
            sp100_temperature_read(i);
            chThdSleepMilliseconds(1);

            sp100_acceleration_measure(i);
            chThdSleepMilliseconds(20);
            status = sp100_status(i);
            sp100_acceleration_read(i);
            chThdSleepMilliseconds(1);

            sp100_pressure_measure(i);
            chThdSleepMilliseconds(20);
            status = sp100_status(i);
            sp100_pressure_read(i);
            chThdSleepMilliseconds(1);

            (void)status;

            chThdSleepMilliseconds(100);
        }
    }
}

void sp100_init(SPIDriver* spid, size_t nss, ioline_t ss[],
                struct SP100 sp100s[]) {
    sp100_spid = spid;
    sp100_nss = nss;
    sp100_ss = ss;
    sp100_out = sp100s;
    chThdCreateStatic(sp100_thd_wa, sizeof(sp100_thd_wa),
                      NORMALPRIO, sp100_thd, NULL);
}
