/*
 *  Martlet 3 Datalogger
 */

#include "ch.h"
#include "hal.h"

#include "logging.h"
#include "pressure.h"
#include "LTC2983.h"
#include "err_handler.h"

#include "m3can.h"
#include "m3status.h"

/* Packet Counter */
static uint32_t pkt_rate;

/* Interrupt Configuration */
static const EXTConfig extcfg = {
  {
    {EXT_CH_MODE_DISABLED, NULL}, /* Px0 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px1 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px2 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px3 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px4 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px5 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px6 */
    {EXT_CH_MODE_RISING_EDGE | EXT_CH_MODE_AUTOSTART | EXT_MODE_GPIOB, temp_ready}, /* PB7 TEMP INT */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px8 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px9 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px10 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Px11 */
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

/* Heartbeat Thread */
static THD_WORKING_AREA(hbt_wa, 128);
static THD_FUNCTION(hbt_thd, arg) {

  (void)arg;
  chRegSetThreadName("Heartbeat");
  
  while (true) {
    
    /* Flash HBT LED */
    palSetPad(GPIOB, GPIOB_LED1_GREEN);
    chThdSleepMilliseconds(100);
    palClearPad(GPIOB, GPIOB_LED1_GREEN);
    chThdSleepMilliseconds(900);
    
    /* Send Current Packet Rate */
    can_send(CAN_MSG_ID_M3DL_RATE, FALSE, (uint8_t*)(&pkt_rate), 4);

    /* Reset Packet Rate Counter */
    pkt_rate = 0;
          
  }
}

/* Function Called on CAN Packet Reception */
void can_recv(uint16_t ID, bool RTR, uint8_t* data, uint8_t len) {

    /* Log Incoming CAN Packet */
    log_can(ID, RTR, len, data);
    
    /* Update Packet Rate */
    pkt_rate += 1;
}


/* Application Entry Point */
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
    
    /* Interrupt Init */
    extStart(&EXTD1, &extcfg);

    /* Datalogging Init */
    logging_init();

    /* Init Heartbeat */
    chThdCreateStatic(hbt_wa, sizeof(hbt_wa), NORMALPRIO, hbt_thd, NULL);

    /* Turn on the CAN System */
    can_init(CAN_ID_M3DL);
        
    /* Enable CAN Feedback */
    can_set_loopback(TRUE);
   
    /* Init m3status */
    m3status_set_init(M3DL_COMPONENT_SD_CARD); 
    m3status_set_init(M3DL_COMPONENT_LTC2983); 
    m3status_set_init(M3DL_COMPONENT_PRESSURE); 

    /* Init LTC2983 */
    ltc2983_init();
    
    /* Init Pressure Sensors */
    pressure_init();
    
    /* Main Loop */
    while (true) {
    
        /* Clear the watchdog timer */
        IWDG->KR = 0xAAAA;

        /* Do nothing */
        chThdSleepMilliseconds(100);
    }
}
