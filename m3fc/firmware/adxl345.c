/*
 * ADXL345 Driver
 * 2014, 2016 Adam Greig, Cambridge University Spaceflight
 */

#include "ch.h"
#include "hal.h"
#include "m3can.h"
#include "m3fc_config.h"
#include "m3fc_status.h"
#include "m3fc_state_estimation.h"
#include "m3fc_mock.h"
#include "adxl345.h"

#define ADXL345_REG_DEVID               0x00
#define ADXL345_REG_BWRATE              0x2C
#define ADXL345_REG_POWER_CTL           0x2D
#define ADXL345_REG_INT_ENABLE          0x2E
#define ADXL345_REG_INT_MAP             0x2F
#define ADXL345_REG_INT_SOURCE          0x30
#define ADXL345_REG_DATA_FORMAT         0x31
#define ADXL345_REG_DATAX0              0x32
#define ADXL345_REG_DATAX1              0x33
#define ADXL345_REG_DATAY0              0x34
#define ADXL345_REG_DATAY1              0x35
#define ADXL345_REG_DATAZ0              0x36
#define ADXL345_REG_DATAZ1              0x37
#define ADXL345_REG_FIFO_CTL            0x38
#define ADXL345_REG_FIFO_STATUS         0x39

#define ADXL345_BWRATE_RATE_800HZ       ((1<<0) | (1<<2) | (1<<3))
#define ADXL345_POWER_CTL_MEASURE       (1<<3)
#define ADXL345_INT_ENABLE_DATA_READY   (1<<7)
#define ADXL345_DATA_FORMAT_RANGE_16G   ((1<<0) | (1<<1))
#define ADXL345_DATA_FORMAT_JUSTIFY     (1<<2)
#define ADXL345_DATA_FORMAT_FULL_RES    (1<<3)
#define ADXL345_DATA_FORMAT_SELF_TEST   (1<<7)

#define ADXL345_WRITE                   (0<<7)
#define ADXL345_READ                    (1<<7)
#define ADXL345_MULTIBYTE               (1<<6)

static bool adxl345_check_id(void);
static void adxl345_read_u8(uint8_t adr, uint8_t* reg);
static void adxl345_write_u8(uint8_t adr, uint8_t val);
static void adxl345_read_accel(int16_t accels[3]);
static void adxl345_configure(void);
static bool adxl345_self_test(void);
static float adxl345_accels_to_up(int16_t accels[3]);

static SPIDriver* adxl345_spid;
static binary_semaphore_t adxl345_thd_sem;

static SPIConfig spi_cfg = {
    .end_cb = NULL,
    .ssport = 0,
    .sspad  = 0,
    .cr1    = SPI_CR1_BR_2 | SPI_CR1_CPOL | SPI_CR1_CPHA
};

/*
 * Read a register at address `adr` on the ADXL345.
 * The register's value is returned via reg.
 */
static void adxl345_read_u8(uint8_t adr, uint8_t* reg)
{
    adr |= ADXL345_READ;
    spiSelect(adxl345_spid);
    spiSend(adxl345_spid, 1, (void*)&adr);
    spiReceive(adxl345_spid, 1, (void*)reg);
    spiUnselect(adxl345_spid);
}

/*
 * Write a register at address `adr` with value `val` on the ADXL345.
 */
static void adxl345_write_u8(uint8_t adr, uint8_t val)
{
    uint8_t tx[2];
    tx[0] = adr | ADXL345_WRITE;
    tx[1] = val;

    spiSelect(adxl345_spid);
    spiSend(adxl345_spid, 2, (void*)tx);
    spiUnselect(adxl345_spid);
}

/*
 * Read the current acceleration values from the ADXL345.
 * The values are stored in `accels` as three int16s.
 */
static void adxl345_read_accel(int16_t* accels)
{
    uint8_t adr = ADXL345_REG_DATAX0 | ADXL345_READ | ADXL345_MULTIBYTE;

    spiSelect(adxl345_spid);
    spiSend(adxl345_spid, 1, (void*)&adr);
    spiReceive(adxl345_spid, 6, (void*)accels);
    spiUnselect(adxl345_spid);
}

/*
 * Run the ADXL345 self test, returns true on success or false on failure.
 */
static bool adxl345_self_test()
{
    int i;
    int16_t accels[3], st_accels[3];
    int32_t accel_sums[3] = {0, 0, 0};

    /* Set 800Hz ODR and disable low powder mode */
    adxl345_write_u8(ADXL345_REG_BWRATE, ADXL345_BWRATE_RATE_800HZ);

    /* Set +-16G range and full resolution */
    adxl345_write_u8(ADXL345_REG_DATA_FORMAT,
        ADXL345_DATA_FORMAT_FULL_RES  |
        ADXL345_DATA_FORMAT_RANGE_16G);

    /* Enable MEASURE mode */
    adxl345_write_u8(ADXL345_REG_POWER_CTL, ADXL345_POWER_CTL_MEASURE);

    /* Discard some samples to allow settling */
    chThdSleepMilliseconds(30);

    /* Read samples */
    for(i=0; i<128; i++) {
        adxl345_read_accel(accels);
        accel_sums[0] += accels[0];
        accel_sums[1] += accels[1];
        accel_sums[2] += accels[2];
        chThdSleepMilliseconds(2);
    }

    /* Store average values */
    accels[0] = accel_sums[0] >> 7;
    accels[1] = accel_sums[1] >> 7;
    accels[2] = accel_sums[2] >> 7;

    /* Clear the sums */
    accel_sums[0] = accel_sums[1] = accel_sums[2] = 0;

    /* Turn on self test at +-16G range, full resolution */
    adxl345_write_u8(ADXL345_REG_DATA_FORMAT,
        ADXL345_DATA_FORMAT_SELF_TEST |
        ADXL345_DATA_FORMAT_FULL_RES  |
        ADXL345_DATA_FORMAT_RANGE_16G);

    /* Discard some samples to allow settling */
    chThdSleepMilliseconds(30);

    /* Read self test samples */
    for(i=0; i<128; i++) {
        adxl345_read_accel(st_accels);
        accel_sums[0] += st_accels[0];
        accel_sums[1] += st_accels[1];
        accel_sums[2] += st_accels[2];
        chThdSleepMilliseconds(2);
    }

    /* Average the samples */
    st_accels[0] = accel_sums[0] >> 7;
    st_accels[1] = accel_sums[1] >> 7;
    st_accels[2] = accel_sums[2] >> 7;

    /* Compute the self test deltas */
    accels[0] = st_accels[0] - accels[0];
    accels[1] = st_accels[1] - accels[1];
    accels[2] = st_accels[2] - accels[2];

    /* ADXL345 self test parameters at 3.3V operation:
     * For X and Y, scale 2.5V figures by 1.77, giving 88LSB shifts,
     * For Z, scale 2.5V figures by 1.47, giving 110LSB shift.
     */
    return (
        accels[0] > 88  &&
        accels[1] < -88 &&
        accels[2] > 112
    );
}

static bool adxl345_check_id()
{
    uint8_t devid;
    adxl345_read_u8(ADXL345_REG_DEVID, &devid);
    return devid == 0xE5;
}

/*
 * Initialise the ADXL345 device.
 * Sets registers for 800Hz operation in high power mode,
 * enables measurement, and runs a self test to verify device performance.
 */
static void adxl345_configure()
{
    /* Set 800Hz ODR and disable low powder mode */
    adxl345_write_u8(ADXL345_REG_BWRATE, ADXL345_BWRATE_RATE_800HZ);

    /* Set +-16G range and full resolution */
    adxl345_write_u8(ADXL345_REG_DATA_FORMAT,
        ADXL345_DATA_FORMAT_FULL_RES  |
        ADXL345_DATA_FORMAT_RANGE_16G);

    /* Enable DATA_READY interrupt on INT1 */
    adxl345_write_u8(ADXL345_REG_INT_ENABLE, ADXL345_INT_ENABLE_DATA_READY);

    /* Enable MEASURE mode */
    adxl345_write_u8(ADXL345_REG_POWER_CTL, ADXL345_POWER_CTL_MEASURE);

    /* Discard some samples to allow settling */
    chThdSleepMilliseconds(30);
}

static float adxl345_accels_to_up(int16_t accels[3]) {
    float accel;

    /* Pick acceleration axis based on configuration */
    if(m3fc_config.profile.accel_axis == M3FC_CONFIG_ACCEL_AXIS_X) {
        accel = (float)accels[0];
    } else if(m3fc_config.profile.accel_axis == M3FC_CONFIG_ACCEL_AXIS_NX) {
        accel = -(float)accels[0];
    } else if(m3fc_config.profile.accel_axis == M3FC_CONFIG_ACCEL_AXIS_Y) {
        accel = (float)accels[1];
    } else if(m3fc_config.profile.accel_axis == M3FC_CONFIG_ACCEL_AXIS_NY) {
        accel = -(float)accels[1];
    } else if(m3fc_config.profile.accel_axis == M3FC_CONFIG_ACCEL_AXIS_Z) {
        accel = (float)accels[2];
    } else if(m3fc_config.profile.accel_axis == M3FC_CONFIG_ACCEL_AXIS_NZ) {
        accel = -(float)accels[2];
    } else {
        if(m3status_get_component(M3FC_COMPONENT_ACCEL) != M3STATUS_ERROR) {
            m3status_set_error(M3FC_COMPONENT_ACCEL, M3FC_ERROR_ACCEL_AXIS);
        }
    }

    /* Convert to m/s (*3.9 to mg, /1000 to g, *9.81 to m/s/s) */
    accel *= ((3.9f / 1000.0f) * 9.80665f);

    /* Remove static acceleration */
    accel -= 9.8066f;

    return accel;
}

/* ISR triggered by the EXTI peripheral when DRDY gets asserted.
 */
void adxl345_interrupt(EXTDriver *extp, expchannel_t channel)
{
    (void)extp;
    (void)channel;

    chSysLockFromISR();
    chBSemSignalI(&adxl345_thd_sem);
    chSysUnlockFromISR();
}

/*
 * ADXL345 main thread.
 */
static THD_WORKING_AREA(adxl345_thd_wa, 256);
static THD_FUNCTION(adxl345_thd, arg)
{
    (void)arg;
    int16_t accels[3];
    msg_t wait_result;

    chRegSetThreadName("ADXL345");
    chBSemObjectInit(&adxl345_thd_sem, false);
    spiStart(adxl345_spid, &spi_cfg);

    while(!adxl345_check_id()) {
        m3status_set_error(M3FC_COMPONENT_ACCEL, M3FC_ERROR_ACCEL_BAD_ID);
        chThdSleepMilliseconds(1000);
    }

    while(!adxl345_self_test()) {
        m3status_set_error(M3FC_COMPONENT_ACCEL, M3FC_ERROR_ACCEL_SELFTEST);
        chThdSleepMilliseconds(1000);
    }

    adxl345_configure();

    while(true) {
        adxl345_read_accel(accels);

        /* If we're doing hardware-in-the-loop mocking, discard the just-read
         * value and use the latest mock value instead.
         */
        if(m3fc_mock_get_enabled()) {
            m3fc_mock_get_accel(accels);
        }

        float accel = adxl345_accels_to_up(accels);
        m3fc_state_estimation_new_accel(accel);

        can_send(CAN_MSG_ID_M3FC_ACCEL, false, (uint8_t*)accels, 6);

        wait_result = chBSemWaitTimeout(&adxl345_thd_sem, MS2ST(100));

        if(wait_result == MSG_TIMEOUT) {
            m3status_set_error(M3FC_COMPONENT_ACCEL, M3FC_ERROR_ACCEL_TIMEOUT);
        } else {
            m3status_set_ok(M3FC_COMPONENT_ACCEL);
        }
    }
}

void adxl345_init(SPIDriver* spid, ioportid_t ssport, uint16_t sspad)
{
    m3status_set_init(M3FC_COMPONENT_ACCEL);

    spi_cfg.ssport = ssport;
    spi_cfg.sspad  = sspad;
    adxl345_spid   = spid;

    chThdCreateStatic(adxl345_thd_wa, sizeof(adxl345_thd_wa),
                      NORMALPRIO, adxl345_thd, NULL);
}
