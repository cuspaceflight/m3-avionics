
#include "ch.h"
#include "hal.h"

#include "psu.h"
#include "status.h"


/* Prototypes */
void get_psu_measurements(void);

/* PSU Status */
psu_status battery;

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
    .smpr1 = ADC_SMPR1_SMP_AN14(ADC_SAMPLE_480) | ADC_SMPR1_SMP_AN15(ADC_SAMPLE_480),
    .smpr2 = ADC_SMPR2_SMP_AN9(ADC_SAMPLE_480),
    .sqr1 = ADC_SQR1_NUM_CH(ADC_NUM_CHANNELS),
    .sqr2 = 0,
    .sqr3 = ADC_SQR3_SQ3_N(ADC_CHANNEL_IN15) |
            ADC_SQR3_SQ2_N(ADC_CHANNEL_IN14) | ADC_SQR3_SQ1_N(ADC_CHANNEL_IN9)
};

/* ADC Readings */
static uint16_t charge_current_voltage;
static uint16_t charge_temp_voltage;


/* Get PSU Measurements */
void get_psu_measurements(void) {

    /* Trigger ADC Conversion */
    adcConvert(&ADCD1, &adcgrpcfg, measure, ADC_BUF_DEPTH);
    chThdSleepMilliseconds(500);
}


/* Set Charging Status - Triggered by Interrupt on CHG_GOOD */
void set_charging_status(EXTDriver *extp, expchannel_t channel) {

	(void)extp;
	(void)channel;
	
	if (palReadPad(GPIOB, GPIOB_CHG_GOOD) == 0) {
		battery.charging = TRUE;
	}

	else if (palReadPad(GPIOB, GPIOB_CHG_GOOD) == 1) {
		battery.charging = FALSE;
	}
}


/* Enable Charging - Triggered by Interrupt on P_GOOD */
void enable_charging(EXTDriver *extp, expchannel_t channel) {
    
    (void)extp;
	(void)channel;
	
	/* Cycle CHG_EN Pin */
    palSetPad(GPIOB, GPIOB_CHG_EN);
    palClearPad(GPIOB, GPIOB_CHG_EN);
}


/* PSU Monitor Thread */
static THD_WORKING_AREA(waPSUThread, 128);
static THD_FUNCTION(PSUThread, arg) {

    (void)arg;
    chRegSetThreadName("PSU");

    /* Start ADC */
    adcStart(&ADCD1, NULL);
    
    /* Init Battery Status */
    battery.charging = FALSE;

    while (true) {

        /* Monitor PSU */
        get_psu_measurements();

        /* Analyse Measurements 
        * N.B. The thermistor is only
        * sourced when the battery is
        * charging. If charging stops 
        * due to an over-temp the 
        * CHG_GOOD pin remains low to
        * indicate charging is still 
        * taking place.
        */

        /* Compute Charging Data */
        if (battery.charging == TRUE) { 
            
            /* Charge Temp in Degrees Celsius */
            charge_temp_voltage = ((measure[0] * 3300) / 4096);
            battery.charge_temp = (((charge_temp_voltage - 300) * 50) / (1200 - 300));
            
            /* Charge Current in mAh */
            charge_current_voltage = ((measure[1] * 3300) / 4096);
            battery.charge_current = ((charge_current_voltage * 400) / 4320);

        } else {

            battery.charge_temp = 0;
            battery.charge_current = 0;
        }

        /* Compute Battery Voltage in mV */
        battery.voltage = ((measure[2] * 6600) / 4096);
        
        /* TODO - Log PSU status */
        
        /* Sleep */
        chThdSleepMilliseconds(1000);
    }
}


/* Init PSU */
void psu_init(void) {
    
    /* Create Thread */
    chThdCreateStatic(waPSUThread, sizeof(waPSUThread), NORMALPRIO, PSUThread, NULL);
}
