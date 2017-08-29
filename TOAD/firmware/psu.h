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

/* PSU Status Struct */
typedef struct __attribute__((packed)) {

    uint16_t charge_current;
    uint16_t voltage;
    uint8_t charge_temp;
    bool charging;
    
} psu_status;

#endif 
