/*
 * Martlet 3 Avionics - Radio Groundstation Blinker Test 
 *
 * N.B. Use of the SYSCFG_CFGR1 register is required to remap PA11/PA12
 *   
 */

#include "ch.h"
#include "hal.h"
#include "si4460.h"

/*
 * LEDs blinker thread, times are in milliseconds.
 */
static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (true) {
    // Update LED pins
    palSetPad(GPIOA, GPIOA_STAT_HBT);
    chThdSleepMilliseconds(100);
    palClearPad(GPIOA, GPIOA_STAT_HBT);
    chThdSleepMilliseconds(400);
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

  struct si4460_config si4460_cfg = {
      .spid = &SPID1,
      .ssport = GPIOA,
      .sspad = GPIOA_SPI1_NSS_RADIO,
      .sdn = false,
      .tcxo = false,
      .xo_freq = 30000000,
      .centre_freq = 869500000,
      .data_rate = 2000,
      .deviation = 1000
  };
  si4460_init(&si4460_cfg);

  /*
   * Normal main() thread activity, in this demo it does nothing.
   */
  while (true) {
	chThdSleepMilliseconds(1000);
  }
}



