/*
 *  m3dl - Blinker Test
 */

#include "ch.h"
#include "hal.h"

/*
 * LEDs blinker thread, times are in milliseconds.
 */
static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (true) {
    // Update LED pins
    palSetPad(GPIOB, GPIOB_LED1);
    palSetPad(GPIOC, GPIOC_LED2);
    chThdSleepMilliseconds(500);
    palClearPad(GPIOB, GPIOB_LED1);
    palClearPad(GPIOC, GPIOC_LED2);
    chThdSleepMilliseconds(500);
  }
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
   * Creates the blinker thread.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

  
  /*
   * Normal main() thread activity, in this demo it does nothing.
   */
  while (true) {
  }
}



