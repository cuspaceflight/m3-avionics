#ifndef LTC2983_H
#define LTC2983_H

/* Init LTC2983 */
void ltc2983_init(void);

/* Interrupt Callback */
void temp_ready(EXTDriver *extp, expchannel_t channel);

#endif
