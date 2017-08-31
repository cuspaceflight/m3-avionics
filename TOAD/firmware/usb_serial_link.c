#include "ch.h"
#include "hal.h"
#include "usbcfg.h"

#include "usb_serial_link.h"
    
/* USB Dump Thread */
static THD_WORKING_AREA(waUSBThread, 2048);
static THD_FUNCTION(USBThread, arg) {

    (void)arg;
    chRegSetThreadName("USB");
    
    sduObjectInit(&SDU1);
    sduStart(&SDU1, &serusbcfg);

    usbDisconnectBus(serusbcfg.usbp);
    chThdSleepMilliseconds(1500);
    usbStart(serusbcfg.usbp, &usbcfg);
    usbConnectBus(serusbcfg.usbp);
    
    while(true) {
    chThdSleepMilliseconds(1500);
    }
    
}

void usb_serial_init(void) {    
    
    chThdCreateStatic(waUSBThread, sizeof(waUSBThread), NORMALPRIO, USBThread, NULL);
}


