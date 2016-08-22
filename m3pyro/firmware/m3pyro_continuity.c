#include "ch.h"
#include "hal.h"
#include "m3can.h"
#include "m3pyro_continuity.h"

static const ADCConversionGroup adc_cfg = {
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
            ADC_SQR3_SQ3_N(ADC_CHANNEL_IN13) |
            ADC_SQR3_SQ4_N(ADC_CHANNEL_IN8),
};

/* 12bit ADC reading of PYRO_CONTINUITY into 0.1ohm/LSB uint8t */
static uint8_t adc_to_resistance(adcsample_t reading) {
    float r = (1000.0f * (float)reading) / (4096.0f - (float)reading);
    if(r > 25.0f) {
        return 255;
    } else {
        return (uint8_t)(r * 10);
    }
}

/* 12bit ADC reading of PYRO_SI into a 100mV/LSB uint8_t */
static uint8_t adc_to_voltage(adcsample_t reading) {
    float v = (15600.0f * 3.3f)/(5600.0f * 4096.0f) * (float)reading;
    return (uint8_t)(v / 0.1f);
}

static THD_WORKING_AREA(m3pyro_continuity_thd_wa, 256);
static THD_FUNCTION(m3pyro_continuity_thd, arg)
{
    (void)arg;
    adcsample_t sampbuf[5];
    adcStart(&ADCD1, NULL);
    while(true) {
        adcConvert(&ADCD1, &adc_cfg, sampbuf, 1);

        uint8_t continuities[4];
        continuities[0] = adc_to_resistance(sampbuf[0]);
        continuities[2] = adc_to_resistance(sampbuf[1]);
        continuities[3] = adc_to_resistance(sampbuf[2]);
        continuities[4] = adc_to_resistance(sampbuf[3]);
        can_send(CAN_MSG_ID_M3PYRO_CONTINUITY, false,
                 continuities, sizeof(continuities));

        uint8_t supply;
        supply = adc_to_voltage(sampbuf[4]);
        can_send(CAN_MSG_ID_M3PYRO_SUPPLY_STATUS, false,
                 &supply, 1);

        /*chThdSleepMilliseconds(500);*/
    }
}

void m3pyro_continuity_init()
{
    chThdCreateStatic(m3pyro_continuity_thd_wa,
                      sizeof(m3pyro_continuity_thd_wa),
                      NORMALPRIO, m3pyro_continuity_thd, NULL);
}
