/*
 * A basic ChibiOS app.
 * Copyright Adam Greig 2011.
 * Released under the GNU GPL v3. Insert GPL boilerplate here.
 */

#include "ch.h"
#include "hal.h"

/*
 * LED control server.
 */
static WORKING_AREA(waLEDS, 128);
static msg_t LEDS(void * arg) {
    Mailbox* mbox = (Mailbox *)arg;
    msg_t msg, result;

    while(TRUE) {
        result = chMBFetch(mbox, &msg, TIME_INFINITE);
        if(result == RDY_OK) {
            if(msg & 1)
                palSetPad(IOPORT2, LED1);
            else
                palClearPad(IOPORT2, LED1);
            if(msg & 2)
                palSetPad(IOPORT2, LED2);
            else
                palClearPad(IOPORT2, LED2);
        }
    }

    return 0;
}

/*
 * LED request client.
 */
static WORKING_AREA(waLEDC, 128);
static msg_t LEDC(void * arg) {
    Mailbox* mbox = (Mailbox *)arg;

    while(TRUE) {
        chMBPost(mbox, 0, TIME_INFINITE);
        chThdSleepMilliseconds(500);
        chMBPost(mbox, 1, TIME_INFINITE);
        chThdSleepMilliseconds(500);
        chMBPost(mbox, 2, TIME_INFINITE);
        chThdSleepMilliseconds(500);
        chMBPost(mbox, 3, TIME_INFINITE);
        chThdSleepMilliseconds(500);
    }

    return 0;
}

/*
 * Application entry point.
 */
int main(void) {

    /*
     * System initializations.
     * - HAL initialization, this also initializes the configured device drivers
     *   and performs the board-specific initializations.
     * - Kernel initialization, the main() function becomes a thread and the
     *   RTOS is active.
     */
    halInit();
    chSysInit();

    /*
     * Create a mailbox for IPC.
     */
    Mailbox mbox;
    msg_t mbox_buffer[3];
    chMBInit(&mbox, mbox_buffer, 3);

    /*
     * Create the LED server and client threads
     */
    chThdCreateStatic(waLEDS, sizeof(waLEDS), NORMALPRIO, LEDS, (void *)&mbox);
    chThdCreateStatic(waLEDC, sizeof(waLEDC), NORMALPRIO, LEDC, (void *)&mbox);

    /*
     * Normal main() thread activity, in this demo it does nothing
     */
    while (TRUE) {
        chThdSleepMilliseconds(500);
    }

    return 0;
}
