
#include "ch.h"
#include "hal.h"

#include "psu.h"

/* PSU Status */
bool input_power = FALSE;
bool battery_charging = FALSE;

/* Prototypes */
void get_psu_status(void);
void get_psu_measurements(void);
void enable_charging(void);

/* ADC Samples */
static adcsample_t measure[ADC_NUM_CHANNELS * ADC_BUF_DEPTH];

/* ADC Config */
static const ADCConversionGroup adcgrpcfg = {
    .circular = FALSE,                         
    .num_channels = ADC_NUM_CHANNELS,        
    .end_cb = NULL,                  
    .error_cb = NULL,
    .cr1 = 0,
    .cr2 = ADC_CR2_SWSTART,
    .smpr1 = ADC_SMPR1_SMP_AN14(ADC_SAMPLE_3) | ADC_SMPR1_SMP_AN15(ADC_SAMPLE_3),
    .smpr2 = ADC_SMPR2_SMP_AN9(ADC_SAMPLE_3),
    .sqr1 = ADC_SQR1_NUM_CH(ADC_NUM_CHANNELS),
    .sqr2 = 0,
    .sqr3 = ADC_SQR3_SQ3_N(ADC_CHANNEL_IN15) |
            ADC_SQR3_SQ2_N(ADC_CHANNEL_IN14)   | ADC_SQR3_SQ1_N(ADC_CHANNEL_IN9)
};


/* Get PSU Measurements */
void get_psu_measurements(void) {

     adcConvert(&ADCD1, &adcgrpcfg, measure, ADC_BUF_DEPTH);
     chThdSleepMilliseconds(500);
}


/* Get PSU Status */
void get_psu_status() {

    /* Read In CHG_GOOD and P_GOOD */
    battery_charging = !(palReadPad(GPIOB, GPIOB_CHG_GOOD));
    input_power = !(palReadPad(GPIOB, GPIOB_POWER_GOOD));

}

/* Enable Charging */
void enable_charging(void) {
    
    /* Cycle CHG_EN Pin */
    palSetPad(GPIOB, GPIOB_CHG_EN);
    chThdSleepMilliseconds(250);
    palClearPad(GPIOB, GPIOB_CHG_EN);
}


/* PSU Monitor Thread */
static THD_WORKING_AREA(waPSUThread, 128);
static THD_FUNCTION(PSUThread, arg) {

  (void)arg;
  chRegSetThreadName("PSU");
  
  /* Start ADC */
  adcStart(&ADCD1, NULL);
  
  while (true) {
    
    /* Monitor PSU */
    get_psu_measurements();
    get_psu_status();
    
    /* TODO: Analyse Measurements */
    
    chThdSleepMilliseconds(5000);
  }
}


/* Init PSU */
void psu_init(void) {
    
    /* Create Thread */
    chThdCreateStatic(waPSUThread, sizeof(waPSUThread), NORMALPRIO, PSUThread, NULL);
}

    
    
