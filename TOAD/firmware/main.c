
#include "ch.h"
#include "hal.h"

#include "psu.h"


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
    {EXT_CH_MODE_FALLING_EDGE | EXT_CH_MODE_AUTOSTART | EXT_MODE_GPIOB, NULL}, /* PB11 */
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
    
    /* TODO Setup HSE */
    
    /* Interrupt Init */
    extStart(&EXTD1, &extcfg);
    
    /* PSU Init */
    psu_init();
    
    /* Main Loop */
    while (true) {
    
        /* Do nothing */
        chThdSleepMilliseconds(1000);
    }
}
