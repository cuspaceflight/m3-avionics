#ifndef M3RADIO_LABRADOR_H
#define M3RADIO_LABRADOR_H

#define M3RADIO_LABRADOR_TXBUFSIZE (128)

#include "hal.h"

void m3radio_labrador_init(void);
void m3radio_labrador_pps_falling(EXTDriver *extp, expchannel_t channel);

#endif
