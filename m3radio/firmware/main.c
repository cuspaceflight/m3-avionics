#include "ch.h"
#include "hal.h"
#include "m3can.h"
#include "m3radio_status.h"
#include "m3radio_gps_ant.h"
#include "m3radio_labrador.h"
#include "m3radio_router.h"
#include "ublox.h"
#include "cs2100.h"

#include <string.h>


/* Interrupt Configuration */
static const EXTConfig extcfg = {
  {
    {EXT_CH_MODE_DISABLED, NULL}, /* Px0 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px1 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px2 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px3 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px4*/
    {EXT_CH_MODE_DISABLED, NULL}, /* Px5 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px6 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px7 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px8 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px9 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px10 */
    {EXT_CH_MODE_FALLING_EDGE | EXT_CH_MODE_AUTOSTART | EXT_MODE_GPIOA, NULL}, /* PA11 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px12 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px13 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px14 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px15 */
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

    /* Turn on the watchdog timer, stopped in debug halt */
    DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP;
    IWDG->KR = 0x5555;
    IWDG->PR = 3;
    IWDG->KR = 0xCCCC;

    /* Initialise ChibiOS */
    halInit();
    chSysInit();

    m3radio_status_init();

    m3radio_gps_ant_init();

    /* Turn on the CAN system and send a packet with our firmware version.
     * We listen to all subsystems so don't set any filters.
     */
    m3can_init(CAN_ID_M3RADIO, NULL, 0);

    /* We'll enable CAN loopback so we can send our own messages over
     * the radio */
    m3can_set_loopback(true);

    m3radio_router_init();

    /* Initialise GPS to produce 1MHz pulse */
    ublox_init(&SD4, true, false, true);

    /* Configure CS2100 to Produce HSE */
    cs2100_configure(&I2CD2);

    /* Swap PLLSRC to HSE */
    cs2100_set_pll();

    /* Interrupt Init */
    extStart(&EXTD1, &extcfg);

    ublox_thd_init();

    m3radio_labrador_init();

    while (true) {
        /* Clear the watchdog timer */
        IWDG->KR = 0xAAAA;

        chThdSleepMilliseconds(100);
    }
}
