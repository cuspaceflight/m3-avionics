#ifndef PSU
#define PSU

/* PSU Measurements */
#define ADC_NUM_CHANNELS   3
#define ADC_BUF_DEPTH      1

/* PSU Init */
void psu_init(void);

/* PSU Callbacks */
void enable_charging(EXTDriver *extp, expchannel_t channel);
void set_charging_status(EXTDriver *extp, expchannel_t channel);

#endif 
