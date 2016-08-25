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

#define SP100_STATUS_OVERFLOW   (1<<0)
#define SP100_STATUS_PARITY     (1<<1)
#define SP100_STATUS_CHECKSUM   (1<<2)
#define SP100_STATUS_PROM       (1<<3)
#define SP100_STATUS_LOWV       (1<<4)
#define SP100_STATUS_FAULT      (1<<5)
#define SP100_STATUS_ACCEL      (1<<6)
#define SP100_STATUS_MEASURE    (1<<7)

static SPIDriver* sp100_spid;
static ioline_t* sp100_ss;
static size_t sp100_nss;
static struct SP100* sp100_out;

static void sp100_spi_start(int n);
static void sp100_spi_stop(void);
static void sp100_reload(int n);
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
static void sp100_wait_measurement(int n);
static void sp100_check_status(int n);

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

static void sp100_reload(int n) {
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

static void sp100_wait_measurement(int n) {
    /* TODO Signal the error over serial too */

    systime_t start_time = chVTGetSystemTime();

    while(sp100_status(n) & SP100_STATUS_MEASURE) {
        /* If we've been waiting more than 50ms, signal an error and
         * continue anyway.
         */
        if(ST2MS(chVTGetSystemTime() - start_time) > 50) {
            palSetLine(LINE_LED_RED);
            return;
        }
    }
}

static void sp100_check_status(int n) {
    /* TODO Signal the error over serial too */
    if(sp100_status(n) & (SP100_STATUS_PARITY   |
                          SP100_STATUS_CHECKSUM |
                          SP100_STATUS_LOWV     |
                          SP100_STATUS_FAULT))
    {
        palSetLine(LINE_LED_RED);
    }
}

static THD_WORKING_AREA(sp100_thd_wa, 256);
static THD_FUNCTION(sp100_thd, arg) {
    (void)arg;
    size_t i;

    /* Wait for SP100 POR */
    chThdSleepMilliseconds(50);

    /* Force reload of OTP of all pressure sensors */
    for(i=0; i<sp100_nss; i++) {
        sp100_reload(i);
        /* Wait until PROM reload bit is cleared */
        while(sp100_status(i) & SP100_STATUS_PROM);
        /* Also wait until status returns all 0s (ignoring accel direction) */
        while(sp100_status(i) & ~SP100_STATUS_ACCEL);
    }

    /* Wait for reload to be done */
    chThdSleepMilliseconds(50);

    /* Read all IDs and supply voltages and accelerations */
    /* Read battery twice to warm up the ADC */
    for(i=0; i<sp100_nss; i++) {
        sp100_battery_measure(i);
        sp100_wait_measurement(i);
        sp100_battery_measure(i);
        sp100_wait_measurement(i);
        sp100_battery_read(i);
        sp100_acceleration_measure(i);
        sp100_wait_measurement(i);
        sp100_acceleration_read(i);
        sp100_id(i);
    }

    /* Go around the loop reading sensors */
    while(true) {
        for(i=0; i<sp100_nss; i++) {
            /* Read temperature */
            sp100_temperature_measure(i);
            sp100_wait_measurement(i);
            sp100_temperature_read(i);
            sp100_check_status(i);

            /* Read pressure */
            sp100_pressure_measure(i);
            sp100_wait_measurement(i);
            sp100_pressure_read(i);
            sp100_check_status(i);

        }
        /* TODO Signal completed read */
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
