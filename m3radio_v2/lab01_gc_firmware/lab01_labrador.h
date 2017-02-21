#ifndef LAB01_LABRADOR_H
#define LAB01_LABRADOR_H

void lab01_labrador_init(void);
void lab01_labrador_run(void);
void lab01_labrador_send(uint16_t msg_id, bool can_rtr,
                         uint8_t* data, uint8_t datalen);

#endif
