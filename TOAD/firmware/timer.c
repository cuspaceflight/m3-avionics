#include "ch.h"
#include "hal.h"

#include "timer.h"
#include "measurements.h"

/* Prototypes */
static void measurements_gpt2_cb(GPTDriver* gptd);


/* Configure GPT2 */
void gpt2_init(void) {

    /* Setup GPT2 driver */ 
    static const GPTConfig gpt2_cfg = {        
    .frequency = 84000000,        
    .callback = measurements_gpt2_cb,        
    .cr2 = 0,        
    .dier = 0,    
    };    
    
    gptStart(&GPTD2, &gpt2_cfg);    
    
    /* Set CC1 and CC2 to input capture mode on TI1 and TI2. */    
    TIM2->CCMR1 = STM32_TIM_CCMR1_CC1S(1) | STM32_TIM_CCMR1_CC2S(1);
        
    /* Enable CC1 and CC2, non-inverted rising-edge. */    
    TIM2->CCER = STM32_TIM_CCER_CC1E | STM32_TIM_CCER_CC2E;    
    
    /* Interrupt on both CC1 (PPS) and CC2 (Radio) */    
    TIM2->DIER = STM32_TIM_DIER_CC1IE | STM32_TIM_DIER_CC2IE;  
      
    /* Start Timer */
    gptStartContinuous(&GPTD2, 0xFFFFFFFF);
    
}


/* Handle TIM2 input capture events */ 
static void measurements_gpt2_cb(GPTDriver* gptd) {    

    (void)gptd;
    
    /* ChibiOS clears TIM2->SR */   
    static uint32_t prev_ccr1 = 0, prev_ccr2 = 0;    

    /* Handle PPS */    
    if(TIM2->CCR1 != prev_ccr1) {        

        prev_ccr1 = TIM2->CCR1;        
        measurements_handle_pps();    
    }    

    /* Handle Radio */    
    if(TIM2->CCR2 != prev_ccr2) {        
    
        prev_ccr2 = TIM2->CCR2;        
        measurements_handle_radio();    
    } 
}
