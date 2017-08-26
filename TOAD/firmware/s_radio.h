#ifndef S_RADIO_H
#define S_RADIO_H

#define SR_LABRADOR_TXBUFSIZE (32)

void sr_labrador_init(void);
void sr_labrador_tx(uint8_t* buf);

#endif
