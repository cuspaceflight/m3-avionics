#include "ch.h"
#include "hal.h"
#include "m3can.h"
#include "m3pyro_continuity.h"
#include "m3pyro_status.h"

static const ADCConversionGroup adc_grp = {
    .circular = false,
    .num_channels = 5,
    .end_cb = NULL,
    .error_cb = NULL,
    .cr1 = 0,
    .cr2 = ADC_CR2_SWSTART,
    .smpr1 = ADC_SMPR1_SMP_AN11(ADC_SAMPLE_144) |
             ADC_SMPR1_SMP_AN13(ADC_SAMPLE_144),
    .smpr2 = ADC_SMPR2_SMP_AN1(ADC_SAMPLE_144) |
             ADC_SMPR2_SMP_AN2(ADC_SAMPLE_144) |
             ADC_SMPR2_SMP_AN8(ADC_SAMPLE_144),
    .sqr1 = ADC_SQR1_NUM_CH(5),
    .sqr2 = 0,
    .sqr3 = ADC_SQR3_SQ1_N(ADC_CHANNEL_IN2) |
            ADC_SQR3_SQ2_N(ADC_CHANNEL_IN1) |
            ADC_SQR3_SQ3_N(ADC_CHANNEL_IN11) |
            ADC_SQR3_SQ4_N(ADC_CHANNEL_IN13) |
            ADC_SQR3_SQ5_N(ADC_CHANNEL_IN8),
};

/* 12bit ADC reading of PYRO_CONTINUITY into 1ohm/LSB uint8t */
/*static uint8_t adc_to_resistance(adcsample_t reading) {
    float r = (1000.0f * (float)reading) / (4096.0f - (float)reading);
    if(r > 250.0f) {
        return 255;
    } else {
        return (uint8_t)r;
    }
}*/

/* 12bit ADC reading of PYRO_SI into a 100mV/LSB uint8_t */
static uint8_t adc_to_voltage(adcsample_t reading) {
    float v = (15600.0f * 3.3f)/(5600.0f * 4096.0f) * (float)reading;
    return (uint8_t)(v / 0.1f);
}

static THD_WORKING_AREA(m3pyro_continuity_thd_wa, 256);
static THD_FUNCTION(m3pyro_continuity_thd, arg)
{
    (void)arg;

    while(true) {
        adcsample_t sampbuf[5];
        adcStart(&ADCD1, NULL);
        msg_t result = adcConvert(&ADCD1, &adc_grp, sampbuf, 1);
        adcStop(&ADCD1);

        if(result != MSG_OK) {
            m3status_set_error(M3PYRO_COMPONENT_CONTINUITY, M3PYRO_ERROR_ADC);
            chThdSleepMilliseconds(500);
            continue;
        }

        //uint8_t continuities[4];
        //continuities[0] = adc_to_resistance(sampbuf[0]);
        //continuities[1] = adc_to_resistance(sampbuf[1]);
        //continuities[2] = adc_to_resistance(sampbuf[2]);
        //continuities[3] = adc_to_resistance(sampbuf[3]);
        //can_send(CAN_MSG_ID_M3PYRO_CONTINUITY, false,
        //         continuities, sizeof(continuities));

        uint8_t raw_readings[8];
	raw_readings[0] = (sampbuf[0] & 0xff);
        raw_readings[1] = (sampbuf[0] >> 8) & 0xff;
	raw_readings[2] = (sampbuf[1] & 0xff);
        raw_readings[3] = (sampbuf[1] >> 8) & 0xff;
	raw_readings[4] = (sampbuf[2] & 0xff);
        raw_readings[5] = (sampbuf[2] >> 8) & 0xff;
	raw_readings[6] = (sampbuf[3] & 0xff);
        raw_readings[7] = (sampbuf[3] >> 8) & 0xff;
        can_send(CAN_MSG_ID_M3PYRO_CONTINUITY, false,
                 raw_readings, sizeof(raw_readings));

        uint8_t supply;
        supply = adc_to_voltage(sampbuf[4]);
        can_send(CAN_MSG_ID_M3PYRO_SUPPLY_STATUS, false,
                 &supply, 1);

        m3status_set_ok(M3PYRO_COMPONENT_CONTINUITY);
        chThdSleepMilliseconds(500);
    }
}

void m3pyro_continuity_init()
{
    m3status_set_init(M3PYRO_COMPONENT_CONTINUITY);
    chThdCreateStatic(m3pyro_continuity_thd_wa,
                      sizeof(m3pyro_continuity_thd_wa),
                      NORMALPRIO, m3pyro_continuity_thd, NULL);
}
