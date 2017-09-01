#ifndef USB_SERIAL_LINK_H
#define USB_SERIAL_LINK_H

/* Start USB Serial Thread */
void usb_serial_init(void);

/* Upload a log message */
void _upload_log(toad_log *packet);

#endif
