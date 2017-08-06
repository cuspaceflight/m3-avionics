#include "ch.h"
#include "hal.h"

#include "timer.h"


static void timer_init(void) {

    /* Configure GPT2 to precisely time the GPS PPS and the mains     
    * zero-crossing events. It runs at full speed 100MHz.     
    * We interrupt on both CC1 (mains zc) and CC2 (PPS).     
    */    
    
    static const GPTConfig gpt2_cfg = {        
    .frequency = TIMER_FREQ,        
    .callback = measurements_gpt2_cb,        
    .cr2 = 0,        
    .dier = 0,    
    };    
    
    gptStart(&GPTD2, &gpt2_cfg);    
    
    /* Set CC1 and CC2 to input capture mode on TI1 and TI2. */    
    TIM2->CCMR1 = STM32_TIM_CCMR1_CC1S(1) | STM32_TIM_CCMR1_CC2S(1);    
    /* Enable CC1 and CC2, non-inverted rising-edge. */    
    TIM2->CCER = STM32_TIM_CCER_CC1E | STM32_TIM_CCER_CC2E;    
    /* Have to set this here rather than in gpt2_cfg as gptStart clears it.. */    
    TIM2->DIER = STM32_TIM_DIER_CC1IE | STM32_TIM_DIER_CC2IE;    
    gptStartContinuous(&GPTD2, 0xFFFFFFFF);
    
}
