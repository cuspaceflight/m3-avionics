/*
 * Low Power Mode
 * Cambridge University Spaceflight
 */

#include "ch.h"

#include "m3can.h"
#include "error.h"
#include "smbus.h"
#include "config.h"
#include "powermanager.h"
#include "chargecontroller.h"
#include "lowpower.h"

static virtual_timer_t lowpower_timer;

static RTCWakeup rtc_wakeup_cfg = {
  (4 << 16) |   // WUCKSEL=ck_spre (1Hz)
  (4)           // Wake every 5s (4+1)
};

void lowpower_init(){
  chVTObjectInit(&lowpower_timer);
}

static bool lowpower_get_entry_flag(void){
  volatile uint32_t *entry = (volatile uint32_t *)(LOWPOWER_ENTRY_FLAG_ADDR);
  return *entry == BACKUPREG_FLAG_MAGIC;
}

static void lowpower_set_entry_flag(bool enabled){
  volatile uint32_t *entry = (volatile uint32_t *)(LOWPOWER_ENTRY_FLAG_ADDR);
  *entry = enabled ? BACKUPREG_FLAG_MAGIC : 0;
}

bool lowpower_get_mode_flag(){
  volatile uint32_t *lpm = (volatile uint32_t *)(LOWPOWER_MODE_FLAG_ADDR);
  return *lpm == BACKUPREG_FLAG_MAGIC;
}

void lowpower_set_mode_flag(bool enabled){
  volatile uint32_t *lpm = (volatile uint32_t *)(LOWPOWER_MODE_FLAG_ADDR);
  *lpm = enabled ? BACKUPREG_FLAG_MAGIC : 0;
}

static void lowpower_set_wakeup_count(uint32_t count){
  volatile uint32_t *wakeup_count = (volatile uint32_t *)(LOWPOWER_WAKEUP_COUNT_ADDR);
  *wakeup_count = count;
}

static uint32_t lowpower_get_wakeup_count(void){
  volatile uint32_t *wakeup_count = (volatile uint32_t *)(LOWPOWER_WAKEUP_COUNT_ADDR);
  return *wakeup_count;
}

void lowpower_early_wakeup_check(){
  // Early power-switch check
  if(palReadLine(LINE_PWR)){ // If the power switch is 'off'
    lowpower_set_mode_flag(false); // Disable low-power mode
    lowpower_set_entry_flag(false);
    lowpower_set_wakeup_count(0); // Clear the wakeup counter

    // Disable battleshort if enabled
    if(ChargeController_get_battleshort_flag()){
      // Bring up the minimal amount to talk to the charger over SMBus
      chSysInit();
      smbus_init(&I2C_DRIVER);
      m3can_init(CAN_ID_M3PSU, NULL, 0);
      ChargeController_init();
      ChargeController_disable_battleshort();
    }

    lowpower_setup_sleep(LOWPOWER_POWER_SWITCH_INTERVAL);
    lowpower_go_to_sleep();
  }

  // If we're in low-power mode
  if(lowpower_get_mode_flag()){

    // If we're not rebooting into low-power mode
    if(!lowpower_get_entry_flag()){
      // Increment the wakeup counter
      lowpower_set_wakeup_count(lowpower_get_wakeup_count() + 1);

      // If we haven't woken up enough times yet, go back to sleep
      if(lowpower_get_wakeup_count() < (LOWPOWER_SLEEP_TIME / LOWPOWER_POWER_SWITCH_INTERVAL)){
        lowpower_setup_sleep(LOWPOWER_POWER_SWITCH_INTERVAL);
        lowpower_go_to_sleep();
      }

      lowpower_set_wakeup_count(0);
    }

    lowpower_set_entry_flag(false);
  }
}

void lowpower_enable(){
  /* Set the flag, then sleep for a few seconds.
   * When we wake from sleep, we'll enter low-power mode as normal
   */
  lowpower_set_mode_flag(true);
  lowpower_set_entry_flag(true);
  lowpower_setup_sleep(1);

  chSysLock();
  while(true){ // Make sure we go to sleep
    lowpower_go_to_sleep();
  }
}

void lowpower_disable(){
  chVTReset(&lowpower_timer); // Cancel the 'awake' timer

  /* Clear the flag, then sleep for a few seconds.
   * When we wake from sleep, we'll be back in full-power mode
   */
  lowpower_set_mode_flag(false);
  lowpower_set_entry_flag(false);
  lowpower_setup_sleep(1);

  chSysLock();
  while(true){ // Make sure we go to sleep
    lowpower_go_to_sleep();
  }
}

static thread_t *wakeup_thp;

static THD_WORKING_AREA(wa_shutdown_thread, 512);
THD_FUNCTION(lowpower_shutdown_thread, arg){
  (void)arg;
  wakeup_thp = chThdGetSelfX();
  while(true){
    chEvtWaitAny((eventmask_t)1);
    PowerManager_shutdown();

    lowpower_setup_sleep(LOWPOWER_POWER_SWITCH_INTERVAL);

    chSysLock();
    while(true){ // Make sure we go to sleep
      lowpower_go_to_sleep();
    }
  }
}

static void lowpower_trigger_shutdown(void *arg){
  (void)arg;
  chSysLockFromISR();
  chEvtSignalI(wakeup_thp, (eventmask_t)1);
  chSysUnlockFromISR();
}

void lowpower_start_awake_timer(){
  // wait some time, then go to sleep again
  chVTSet(&lowpower_timer, S2ST(LOWPOWER_AWAKE_TIME), lowpower_trigger_shutdown, NULL);
  chThdCreateStatic(wa_shutdown_thread, sizeof(wa_shutdown_thread), NORMALPRIO, lowpower_shutdown_thread, NULL);
}

void lowpower_setup_sleep(uint16_t seconds){
  if(seconds == 0){
    rtcSTM32SetPeriodicWakeup(&RTCD1, NULL); // Clear wakeup
  }else{
    rtc_wakeup_cfg.wutr = (4 << 16) | (seconds-1);
    rtcSTM32SetPeriodicWakeup(&RTCD1, &rtc_wakeup_cfg);
  }
}

void lowpower_go_to_sleep(void){
  // Enable deep-sleep
  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
  // Enable STANDBY mode
  PWR->CR |= PWR_CR_PDDS;
  // Make sure STOP mode is disabled
  PWR->CR &= ~PWR_CR_LPDS;

  // Clear the WUF Wakeup Flag
  PWR->CR |= PWR_CR_CWUF;

  // Enabled the Backup SRAM supply
  PWR->CSR |= PWR_CSR_BRE;

  // Clear any RTC interrupts
  RTC->ISR &= ~(RTC_ISR_ALRBF | RTC_ISR_ALRAF | RTC_ISR_WUTF | RTC_ISR_TAMP1F |
    RTC_ISR_TSOVF | RTC_ISR_TSF);

  __SEV(); // Make sure there is an event present
  __WFE(); // Clear the event
  __WFE(); // Go to sleep
}

THD_FUNCTION(lowpower_power_check_thread, arg){
  (void)arg;
  chRegSetThreadName("Power Switch Checker");

  while(true){
    bool switch_open = palReadLine(LINE_PWR);
    if(switch_open){
      PowerManager_shutdown();

      lowpower_setup_sleep(LOWPOWER_POWER_SWITCH_INTERVAL);

      chSysLock();
      while(true){ // Make sure we go to sleep
        lowpower_go_to_sleep();
      }

    }
    chThdSleepMilliseconds(1000);
  }
}

