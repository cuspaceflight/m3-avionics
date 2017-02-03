#include "ch.h"
#include "hal.h"

#include "chprintf.h"

#include "lab01_labrador.h"
#include "usbserial.h"

/* Swap from HSI to HSE (13MHz externally driven signal) */
void clk_swap(void)
{
    chSysLock();

    /* Turn on HSE with bypass and clock security. */
    RCC->CR |= RCC_CR_HSEON | RCC_CR_HSEBYP | RCC_CR_CSSON;
    /* Wait for HSE to be stable. */
    while((RCC->CR & RCC_CR_HSERDY) == 0);

    /* Swap the SYSCLK to HSI */
    RCC->CFGR &= ~RCC_CFGR_SW;
    /* Wait for HSI to be the system clock */
    while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI);

    /* Turn off the PLL */
    RCC->CR &= ~RCC_CR_PLLON;

    /* Set PLL input to HSE, M to HSE/1=13, default N, P, Q. */
    RCC->PLLCFGR = STM32_PLLQ | STM32_PLLP | STM32_PLLN |
                   STM32_PLLSRC_HSE | 13;

    /* Enable PLL */
    RCC->CR |= RCC_CR_PLLON;
    /* Wait for voltage regulator to be stable */
    while((PWR->CSR & PWR_CSR_VOSRDY) == 0);
    /* Note that overdrive mode should still be enabled from ChibiOS. */
    /* Wait for PLL lock. */
    while(!(RCC->CR & RCC_CR_PLLRDY));

    /* Swap system clock back to PLL. */
    RCC->CFGR |= STM32_SW_PLL;
    /* Wait for PLL to be used as system clock. */
    while((RCC->CFGR & RCC_CFGR_SWS) != (STM32_SW_PLL << 2));

    chSysUnlock();
}

int main(void) {
    halInit();
    chSysInit();

    palSetLineMode(LINE_PIO0, PAL_MODE_OUTPUT_PUSHPULL);
    palSetLineMode(LINE_PIO1, PAL_MODE_OUTPUT_PUSHPULL);
    palSetLineMode(LINE_PIO2, PAL_MODE_OUTPUT_PUSHPULL);
    palClearLine(LINE_PIO0);
    palClearLine(LINE_PIO1);
    palClearLine(LINE_PIO2);

    lab01_labrador_init();

    /* Wait for clock out on Si446x */
    chThdSleepMilliseconds(100);

    /* Swap to using the Si446x clock */
    clk_swap();

    usbserial_init();
    lab01_labrador_run();

    while (true) {
        palClearLine(LINE_PIO2);
        chThdSleepMilliseconds(500);
        palSetLine(LINE_PIO2);
        chThdSleepMilliseconds(500);
    }
}

/* On hard fault, copy HARDFAULT_PSP to the sp reg so gdb can give a trace */
void **HARDFAULT_PSP;
register void *stack_pointer asm("sp");
void HardFault_Handler(void) {
    asm("mrs %0, psp" : "=r"(HARDFAULT_PSP) : :);
    stack_pointer = HARDFAULT_PSP;
    GPIOA->ODR |= 1;
    while(1);
}
