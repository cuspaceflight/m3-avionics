#include "ch.h"
#include "hal.h"
#include "m3pyro_hal.h"
#include "m3pyro_status.h"

static const ADCConversionGroup adc_grp_cont = {
    .circular = false,
    .num_channels = 1,
    .end_cb = NULL,
    .error_cb = NULL,
    .cr1 = 0,
    .cr2 = ADC_CR2_SWSTART,
    .smpr1 = 0,
    .smpr2 = ADC_SMPR2_SMP_AN6(ADC_SAMPLE_480),
    .sqr1 = ADC_SQR1_NUM_CH(1),
    .sqr2 = 0,
    .sqr3 = ADC_SQR3_SQ1_N(ADC_CHANNEL_IN6),
};

static const ADCConversionGroup adc_grp_supply = {
    .circular = false,
    .num_channels = 1,
    .end_cb = NULL,
    .error_cb = NULL,
    .cr1 = 0,
    .cr2 = ADC_CR2_SWSTART,
    .smpr1 = 0,
    .smpr2 = ADC_SMPR2_SMP_AN4(ADC_SAMPLE_480),
    .sqr1 = ADC_SQR1_NUM_CH(1),
    .sqr2 = 0,
    .sqr3 = ADC_SQR3_SQ1_N(ADC_CHANNEL_IN4),
};

static const ADCConversionGroup adc_grp_bus = {
    .circular = false,
    .num_channels = 1,
    .end_cb = NULL,
    .error_cb = NULL,
    .cr1 = 0,
    .cr2 = ADC_CR2_SWSTART,
    .smpr1 = 0,
    .smpr2 = ADC_SMPR2_SMP_AN5(ADC_SAMPLE_480),
    .sqr1 = ADC_SQR1_NUM_CH(1),
    .sqr2 = 0,
    .sqr3 = ADC_SQR3_SQ1_N(ADC_CHANNEL_IN5),
};

/* Convert the 12bit ADC reading into a 100mV/LSB voltage. */
static uint8_t adc_to_voltage(adcsample_t reading) {
    float v = (13300.0f * 3.3f)/(3300.0f * 4096.0f) * (float)reading;
    return (uint8_t)(v / 0.1f);
}

/* Initialise the HAL. Sets up ADCs etc. */
void m3pyro_hal_init(void) {
    m3status_set_init(M3PYRO_COMPONENT_HAL);
    adcStart(&ADCD1, NULL);
    adcStart(&ADCD2, NULL);

    /* Just make really sure everything is off. */
    palClearLine(LINE_1A_EN);
    palClearLine(LINE_3A_EN);
    palClearLine(LINE_CONT_EN);
    palClearLine(LINE_CH1);
    palClearLine(LINE_CH2);
    palClearLine(LINE_CH3);
    palClearLine(LINE_CH4);
    palClearLine(LINE_CH5);
    palClearLine(LINE_CH6);
    palClearLine(LINE_CH7);
    palClearLine(LINE_CH8);
}

/* Read bus voltage. Returns in units of 0.1V, e.g. 33 for 3.3V. */
uint8_t m3pyro_read_bus(void) {
    static adcsample_t samp;

    adcAcquireBus(&ADCD2);
    msg_t result = adcConvert(&ADCD2, &adc_grp_bus, &samp, 1);
    adcReleaseBus(&ADCD2);

    if(result != MSG_OK) {
        m3status_set_error(M3PYRO_COMPONENT_HAL, M3PYRO_ERROR_ADC);
        return 0xFF;
    } else {
        m3status_set_ok(M3PYRO_COMPONENT_HAL);
    }
    return adc_to_voltage(samp);
}

/* Read supply voltage. Returns in units of 0.1V, e.g. 74 for 7.4V. */
uint8_t m3pyro_read_supply(void) {
    static adcsample_t samp;

    adcAcquireBus(&ADCD2);
    msg_t result = adcConvert(&ADCD2, &adc_grp_supply, &samp, 1);
    adcReleaseBus(&ADCD2);

    if(result != MSG_OK) {
        m3status_set_error(M3PYRO_COMPONENT_HAL, M3PYRO_ERROR_ADC);
        return 0xFF;
    } else {
        m3status_set_ok(M3PYRO_COMPONENT_HAL);
    }
    return adc_to_voltage(samp);
}

/* Read continuity. Returns high resolution raw ADC reading. */
adcsample_t m3pyro_read_cont(void) {
    static adcsample_t samp;

    adcAcquireBus(&ADCD1);
    msg_t result = adcConvert(&ADCD1, &adc_grp_cont, &samp, 1);
    adcReleaseBus(&ADCD1);

    if(result != MSG_OK) {
        m3status_set_error(M3PYRO_COMPONENT_HAL, M3PYRO_ERROR_ADC);
        return 0xFFFF;
    } else {
        m3status_set_ok(M3PYRO_COMPONENT_HAL);
    }
    return samp;
}

/* Enable the continuity measurement current. */
void m3pyro_cont_enable(void) {
    palSetLineMode(LINE_CONT_EN, PAL_MODE_OUTPUT_PUSHPULL);
    palSetLine(LINE_CONT_EN);
}

/* Disable the continuity measurement current, setting the pin hi-z. */
void m3pyro_cont_disable(void) {
    palSetLineMode(LINE_CONT_EN, PAL_MODE_INPUT);
}

/* Use the continuity measurement to discharge the bus. */
void m3pyro_cont_gnd(void) {
    palSetLineMode(LINE_CONT_EN, PAL_MODE_OUTPUT_PUSHPULL);
    palClearLine(LINE_CONT_EN);
}

/* Assert a channel, connecting it to the bus. */
void m3pyro_assert_ch(uint8_t channel) {
    palClearLine(LINE_CH1);
    palClearLine(LINE_CH2);
    palClearLine(LINE_CH3);
    palClearLine(LINE_CH4);
    palClearLine(LINE_CH5);
    palClearLine(LINE_CH6);
    palClearLine(LINE_CH7);
    palClearLine(LINE_CH8);

    if(channel == 1) {
        palSetLine(LINE_CH1);
    } else if(channel == 2) {
        palSetLine(LINE_CH2);
    } else if(channel == 3) {
        palSetLine(LINE_CH3);
    } else if(channel == 4) {
        palSetLine(LINE_CH4);
    } else if(channel == 5) {
        palSetLine(LINE_CH5);
    } else if(channel == 6) {
        palSetLine(LINE_CH6);
    } else if(channel == 7) {
        palSetLine(LINE_CH7);
    } else if(channel == 8) {
        palSetLine(LINE_CH8);
    } else {
        m3status_set_error(M3PYRO_COMPONENT_HAL, M3PYRO_ERROR_BADCH);
    }
}

/* De-assert all channels, disconnecting them from the bus. */
void m3pyro_deassert_ch(void) {
    palClearLine(LINE_CH1);
    palClearLine(LINE_CH2);
    palClearLine(LINE_CH3);
    palClearLine(LINE_CH4);
    palClearLine(LINE_CH5);
    palClearLine(LINE_CH6);
    palClearLine(LINE_CH7);
    palClearLine(LINE_CH8);
}

/* Enable the 1A constant current supply, energizing the bus. */
void m3pyro_1a_enable(void) {
    palSetLine(LINE_1A_EN);
}

/* Disable the 1A constant current supply. */
void m3pyro_1a_disable(void) {
    palClearLine(LINE_1A_EN);
}

/* Enable the 3A constant current supply, energizing the bus. */
void m3pyro_3a_enable(void) {
    palSetLine(LINE_3A_EN);
}

/* Disable the 3A constant current supply. */
void m3pyro_3a_disable(void) {
    palClearLine(LINE_3A_EN);
}
