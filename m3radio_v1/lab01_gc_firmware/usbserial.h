#ifndef USBSERIAL_H
#define USBSERIAL_H

void usbserial_init(void);
void usbserial_send(uint16_t sid, uint8_t rtr, uint8_t* data, uint8_t dlc);

#endif
