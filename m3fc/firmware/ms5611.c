#include "ch.h"
#include "hal.h"
#include "m3can.h"
#include "m3fc_status.h"
#include "m3fc_state_estimation.h"
#include "m3fc_mock.h"
#include "ms5611.h"

#define MS5611_CMD_RESET                0x1E
#define MS5611_CMD_CONVERT_D1_OSR256    0x40
#define MS5611_CMD_CONVERT_D1_OSR512    0x42
#define MS5611_CMD_CONVERT_D1_OSR1024   0x44
#define MS5611_CMD_CONVERT_D1_OSR2048   0x46
#define MS5611_CMD_CONVERT_D1_OSR4096   0x48
#define MS5611_CMD_CONVERT_D2_OSR256    0x50
#define MS5611_CMD_CONVERT_D2_OSR512    0x52
#define MS5611_CMD_CONVERT_D2_OSR1024   0x54
#define MS5611_CMD_CONVERT_D2_OSR2048   0x56
#define MS5611_CMD_CONVERT_D2_OSR4096   0x58
#define MS5611_CMD_ADC_READ             0x00
#define MS5611_CMD_PROM_READ            0xA0

typedef struct {
    uint16_t c1, c2, c3, c4, c5, c6;
} MS5611CalData;

static SPIDriver* ms5611_spid;

static void ms5611_reset(void);
static void ms5611_read_u16(uint8_t adr, uint16_t* c);
static void ms5611_read_s24(uint8_t adr, int32_t* d);
static void ms5611_read_cal(MS5611CalData* cal_data);
static void ms5611_read(MS5611CalData* cal_data,
                        int32_t* temperature, int32_t* pressure);

static SPIConfig spi_cfg = {
    .end_cb = NULL,
    .ssport = 0,
    .sspad  = 0,
    .cr1    = SPI_CR1_BR_1 | SPI_CR1_BR_2 | SPI_CR1_CPOL | SPI_CR1_CPHA
};

/*
 * Resets the MS5611. Sends 0x1E, waits 5ms.
 */
static void ms5611_reset()
{
    uint8_t adr = MS5611_CMD_RESET;
    spiSelect(ms5611_spid);
    spiSend(ms5611_spid, 1, (void*)&adr);
    chThdSleepMilliseconds(5);
    spiUnselect(ms5611_spid);
}

/*
 * Reads a uint16 from the MS5611 address `adr`, stores it to `c`.
 */
static void ms5611_read_u16(uint8_t adr, uint16_t* c)
{
    uint8_t rx[2];
    spiSelect(ms5611_spid);
    spiSend(ms5611_spid, 1, (void*)&adr);
    spiReceive(ms5611_spid, 2, (void*)rx);
    spiUnselect(ms5611_spid);
    *c = rx[0] << 8 | rx[1];
}

/*
 * Reads an int24 from the MS5611 address `adr`, stores it to `d`.
 */
static void ms5611_read_s24(uint8_t adr, int32_t* d)
{
    uint8_t adc_adr = MS5611_CMD_ADC_READ, rx[3];

    spiSelect(ms5611_spid);
    spiSend(ms5611_spid, 1, (void*)&adr);

    /*
     * Wait for conversion to complete. There doesn't appear to be any way
     * to do this without timing it, unfortunately.
     */
    chThdSleepMicroseconds(600);
    spiUnselect(ms5611_spid);

    /* Read ADC result */
    spiSelect(ms5611_spid);
    spiSend(ms5611_spid, 1, (void*)&adc_adr);
    spiReceive(ms5611_spid, 3, (void*)rx);
    spiUnselect(ms5611_spid);

    *d = rx[0] << 16 | rx[1] << 8 | rx[2];
}

/*
 * Reads MS5611 calibration data, writes it to `cal_data`.
 */
static void ms5611_read_cal(MS5611CalData* cal_data)
{
    uint16_t d0, d7;
    ms5611_read_u16(MS5611_CMD_PROM_READ +  0, &d0);
    ms5611_read_u16(MS5611_CMD_PROM_READ +  2, &(cal_data->c1));
    ms5611_read_u16(MS5611_CMD_PROM_READ +  4, &(cal_data->c2));
    ms5611_read_u16(MS5611_CMD_PROM_READ +  6, &(cal_data->c3));
    ms5611_read_u16(MS5611_CMD_PROM_READ +  8, &(cal_data->c4));
    ms5611_read_u16(MS5611_CMD_PROM_READ + 10, &(cal_data->c5));
    ms5611_read_u16(MS5611_CMD_PROM_READ + 12, &(cal_data->c6));
    ms5611_read_u16(MS5611_CMD_PROM_READ + 14, &d7);
}

/*
 * Read and compensate a temperature and pressure from the MS5611.
 *
 * `cal_data` is previously read calibration data.
 * `temperature` and `pressure` are written to.
 *
 * `temperature` is in centidegrees Celcius,
 * `pressure` is in Pascals.
 */
static void ms5611_read(MS5611CalData* cal_data,
                        int32_t* temperature, int32_t* pressure)
{
    int32_t d1, d2;
    int64_t off, sens, dt;
    int64_t t2 = 0, sens2 = 0, off2 = 0;
    ms5611_read_s24(MS5611_CMD_CONVERT_D1_OSR256, &d1);
    ms5611_read_s24(MS5611_CMD_CONVERT_D2_OSR256, &d2);

    /* Compute temperature */
    dt = (int64_t)d2 - ((int64_t)cal_data->c5 << 8);
    *temperature = 2000 + ((dt * (int64_t)cal_data->c6) >> 23);

    /* Compute offset and sensitivity */
    off = ((int64_t)cal_data->c2 << 16) + (((int64_t)cal_data->c4 * dt) >> 7);
    sens = ((int64_t)cal_data->c1 << 15) + (((int64_t)cal_data->c3 * dt) >> 8);

    /* Perform low temperature compensation */
    if(*temperature < 2000) {
        t2 = (dt * dt) >> 31;
        off2 = 5 * (*temperature - 2000)*(*temperature - 2000) >> 1;
        sens2 = off2 >> 1;
        if(*temperature < -1500) {
            off2 += 7 * (*temperature + 1500)*(*temperature + 1500);
            sens2 += 11 * (*temperature + 1500)*(*temperature + 1500) >> 1;
        }
        *temperature -= t2;
        off -= off2;
        sens -= sens2;
    }

    /* Compute and store new pressure and temperature */
    *pressure = (((d1 * sens) >> 21) - off) >> 15;
}

/*
 * MS5611 main thread.
 * Resets the MS5611, reads cal data, then reads a pressure and temperature
 * in a loop.
 */
static THD_WORKING_AREA(ms5611_thd_wa, 256);
static THD_FUNCTION(ms5611_thd, arg) {
    (void)arg;

    MS5611CalData cal_data;
    int32_t temperature, pressure;
    int loopcount = 0;
    chRegSetThreadName("MS5611");
    spiStart(ms5611_spid, &spi_cfg);
    ms5611_reset();
    ms5611_read_cal(&cal_data);
    while (true) {
        ms5611_read(&cal_data, &temperature, &pressure);

        if(m3fc_mock_get_enabled()) {
            m3fc_mock_get_baro(&pressure, &temperature);
        }

        m3fc_state_estimation_new_pressure((float)pressure);
        if(loopcount++ == 100) {
            uint32_t buf[2] = {temperature, pressure};
            can_send(CAN_MSG_ID_M3FC_BARO, false, (uint8_t*)buf, 8);
            loopcount = 0;
        }
        m3status_set_ok(M3FC_COMPONENT_BARO);
    }
}

void ms5611_init(SPIDriver* spid, ioportid_t ssport, uint16_t sspad) {
    m3status_set_init(M3FC_COMPONENT_BARO);

    spi_cfg.ssport = ssport;
    spi_cfg.sspad  = sspad;
    ms5611_spid    = spid;

    chThdCreateStatic(ms5611_thd_wa, sizeof(ms5611_thd_wa),
                      NORMALPRIO, ms5611_thd, NULL);
}
