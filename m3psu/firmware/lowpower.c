/*
 * Low Power Mode
 * Cambridge University Spaceflight
 */

#include "ch.h"

#include "error.h"

#include "powermanager.h"
#include "lowpower.h"

static virtual_timer_t lowpower_timer;

static RTCWakeup rtc_wakeup_cfg = {
  (4 << 16) |   // WUCKSEL=ck_spre (1Hz)
  (4)           // Wake every 5s (4+1)
};

void lowpower_init(){
  chVTObjectInit(&lowpower_timer);
}

void lowpower_enable(){
  /* Set the flag, then sleep for a few seconds.
   * When we wake from sleep, we'll enter low-power mode as normal
   */
   lowpower_set_mode_flag(true);
   lowpower_setup_sleep(3);
   lowpower_go_to_sleep();
}

void lowpower_disable(){
  chVTReset(&lowpower_timer); // Cancel the 'awake' timer
}

static void lowpower_mode_sleep(void *arg){
  (void)arg;
  PowerManager_shutdown();

  lowpower_setup_sleep(LOWPOWER_SLEEP_TIME);

  chSysLock();
  while(true){ // Make sure we go to sleep
    lowpower_go_to_sleep();
  }
}

void lowpower_start_awake_timer(){
  // wait 3 mins, then go to sleep again
  chVTSet(&lowpower_timer, S2ST(LOWPOWER_AWAKE_TIME), lowpower_mode_sleep, NULL);
}

bool lowpower_get_mode_flag(){
  volatile uint32_t *lpm = (volatile uint32_t *)(LOW_POWER_MODE_FLAG_ADDR);
  return *lpm == 0x12345678;
}

void lowpower_set_mode_flag(bool enabled){
  volatile uint32_t *lpm = (volatile uint32_t *)(LOW_POWER_MODE_FLAG_ADDR);
  *lpm = enabled ? 0x12345678 : 0;
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
