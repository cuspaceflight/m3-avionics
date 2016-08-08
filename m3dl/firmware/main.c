/*
 *  Martlet 3 Data Logger
 */

#include "ch.h"
#include "hal.h"
#include "LTC2983.h"
#include "err_handler.h"


/* Heartbeat Thread*/
static THD_WORKING_AREA(hbt_wa, 128);
static THD_FUNCTION(hbt_thd, arg) {

  (void)arg;
  chRegSetThreadName("Heartbeat");
  while (true) {
    /* Flash HBT LED */
    palSetPad(GPIOB, GPIOB_LED1_GREEN);
    chThdSleepMilliseconds(500);
    palClearPad(GPIOB, GPIOB_LED1_GREEN);
    chThdSleepMilliseconds(500);
  }
}


/* Application entry point */
int main(void) {

  /* System Init */
  halInit();
  chSysInit();

  /*  LTC2983 Init */
  ltc2983_init();	

  /* Init Heartbeat */
  chThdCreateStatic(hbt_wa, sizeof(hbt_wa), NORMALPRIO, hbt_thd, NULL);

 
  while (true) {
      /* Do nothing */
  }

}



