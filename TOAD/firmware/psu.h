#ifndef PSU
#define PSU

/* PSU Measurements */
#define ADC_NUM_CHANNELS   3
#define ADC_BUF_DEPTH      1

/* PSU Init */
void psu_init(void);
void enable_charging(void);

#endif 
