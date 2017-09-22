#include "ch.h"
#include "hal.h"

#include "cs2100.h"
#include "gps.h"
#include "psu.h"
#include "config.h"
#include "s_radio.h"
#include "timer.h"
#include "status.h"
#include "microsd.h"
#include "downlink.h"
#include "logging.h"
#include "measurements.h"
#include "usb_serial_link.h"

/* nIRQ is configured to rise when sync is detected,
 * so then enable responding to rxclk events.
 */
void pr_nirq_isr(EXTDriver *extp, expchannel_t channel)
{
    (void)extp; (void)channel;
    gpt2_enable_ccr2();
}


/* Interrupt Configuration */
static const EXTConfig extcfg = {
  {
    {EXT_CH_MODE_DISABLED, NULL}, /* Px0 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px1 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px2 */
    {EXT_CH_MODE_BOTH_EDGES | EXT_CH_MODE_AUTOSTART | EXT_MODE_GPIOB, set_charging_status}, /* PB3 - CHG_GOOD */
    {EXT_CH_MODE_FALLING_EDGE | EXT_CH_MODE_AUTOSTART | EXT_MODE_GPIOB, enable_charging}, /* PB4 - P_GOOD */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px5 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px6 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px7 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px8 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px9 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px10 */
    {EXT_CH_MODE_RISING_EDGE | EXT_CH_MODE_AUTOSTART | EXT_MODE_GPIOB, pr_nirq_isr}, /* PB11 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px12 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px13 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px14 */
    {EXT_CH_MODE_FALLING_EDGE | EXT_CH_MODE_AUTOSTART | EXT_MODE_GPIOC, NULL}, /* PC15 */
    {EXT_CH_MODE_DISABLED, NULL}, /* PVD Wakeup */
    {EXT_CH_MODE_DISABLED, NULL}, /* RTC Alarm Event */
    {EXT_CH_MODE_DISABLED, NULL}, /* USB OTG FS Wakeup */
    {EXT_CH_MODE_DISABLED, NULL}, /* Ethernet Wakeup */
    {EXT_CH_MODE_DISABLED, NULL}, /* USB OTG HS Wakeup */
    {EXT_CH_MODE_DISABLED, NULL}, /* RTC Timestamp */
    {EXT_CH_MODE_DISABLED, NULL}  /* RTC Wakeup */
  }
};


int main(void) {

    /* Allow debug access during WFI sleep */
    DBGMCU->CR |= DBGMCU_CR_DBG_SLEEP;

    /* Initialise ChibiOS */
    halInit();
    chSysInit();

    /* Configure GPS to Produce 1MHz Signal */
    gps_init(&SD1, true, false, true);

    /* Configure CS2100 to Produce HSE */
    cs2100_configure(&I2CD1);

    /* Swap PLLSRC to HSE */
    cs2100_set_pll();

    /* Interrupt Init */
    extStart(&EXTD1, &extcfg);

    /* Start Logging Thread */
    logging_init();

    /* Start PSU */
    psu_init();

    /* Start Measurement Handler */
    measurement_init();

    /* Start Timer */
    gpt2_init();

    /* Start GPS State Machine */
    gps_thd_init();

    /* Start Secondary Radio */
    sr_labrador_init();

    /* Start Primary Radio */
    downlink_init();

    /* Start USB System */
    usb_serial_init();

    /* Update System Status */
    set_status(COMPONENT_SYS, STATUS_GOOD);

    /* Main Loop */
    while (true) {

        /* Do nothing */
        chThdSleepMilliseconds(1000);
    }
}
