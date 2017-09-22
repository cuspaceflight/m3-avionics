#include "ch.h"
#include "hal.h"

#include "usbcfg.h"
#include "chprintf.h"
#include <string.h>
#include "logging.h"
#include "status.h"
#include "packets.h"
#include "config.h"
#include "usb_serial_link.h"

#define USB_MEMPOOL_ITEMS 64

/* Function Prototypes */
static void mem_init(void);
static void usb_driver_init(void);

/* Memory pool to store incoming data 
 * before being spat out over USB.
 */
static memory_pool_t usb_mempool;

/* Mailbox for storing pointers to queued data */
static mailbox_t usb_mailbox;

/* Statically allocated memory used for the memory pool */
static volatile char usb_mempool_buffer[USB_MEMPOOL_ITEMS * sizeof(toad_log)]
                     __attribute__((aligned(sizeof(stkalign_t))))
                     __attribute__((section(".ccm")));
                     
/* Statically allocated memory used for the queue in mailbox */
static volatile msg_t usb_mailbox_buffer[USB_MEMPOOL_ITEMS]
                      __attribute__((section(".ccm")));
                                                
    
/* USB Serial Thread */
static THD_WORKING_AREA(waUSBThread, 2048);
static THD_FUNCTION(USBThread, arg) {

    (void)arg;
    chRegSetThreadName("USB");
    
    /* Packet Size */
    static const int packet_size = sizeof(toad_log);
          
    /* Mailbox Variables */             
    msg_t mailbox_res;       
    intptr_t data_msg;     

    /* Initalise Memory */
    mem_init();
    
    /* Initalise USB Serial */
    usb_driver_init();
    
    
    while (true) {

        /* Wait for message to be avaliable */
        mailbox_res = chMBFetch(&usb_mailbox, (msg_t*)&data_msg, MS2ST(100));

        /* Re-attempt if mailbox was reset or fetch failed */
        if (mailbox_res != MSG_OK || data_msg == 0) continue;

        /* Spit out queued message and free from memory pool */
        chnWriteTimeout(&SDU1, (void*)data_msg, packet_size, MS2ST(100));
        chPoolFree(&usb_mempool, (void*)data_msg);
    }    
}


/* Initialise Mailbox and Memorypool */
static void mem_init(void) {
    
    chMBObjectInit(&usb_mailbox, (msg_t*)usb_mailbox_buffer, USB_MEMPOOL_ITEMS);
    chPoolObjectInit(&usb_mempool, sizeof(toad_log), NULL);

    /* Fill Memory Pool with Statically Allocated Bits of Memory */
    chPoolLoadArray(&usb_mempool, (void*)usb_mempool_buffer, USB_MEMPOOL_ITEMS);
}


/* Initialise Serial USB Objects */
static void usb_driver_init(void) {

    /* Serial Driver Setup */
    sduObjectInit(&SDU1);
    sduStart(&SDU1, &serusbcfg);

    /* USB Setup */
    usbDisconnectBus(serusbcfg.usbp);
    chThdSleepMilliseconds(1500);
    usbStart(serusbcfg.usbp, &usbcfg);
    usbConnectBus(serusbcfg.usbp);    
}


/* Start USB Serial Thread */
void usb_serial_init(void) {    
    
    chThdCreateStatic(waUSBThread, sizeof(waUSBThread), NORMALPRIO, USBThread, NULL);
}


/* Pass a formatted packet to the USB vomit thread */
void _upload_log(toad_log *packet) {

    void* msg;
    msg_t retval;

    /* Allocate space for the packet in the mempool */
    msg = chPoolAlloc(&usb_mempool);
    if (msg == NULL) return;
    
    /* Copy the packet into the mempool */
    memcpy(msg, (void*)packet, sizeof(toad_log));
    
    /* Post the location of the packet into the mailbox */
    retval = chMBPost(&usb_mailbox, (intptr_t)msg, TIME_IMMEDIATE);
    if (retval != MSG_OK) {
        chPoolFree(&usb_mempool, msg);
        return;
    }
}
