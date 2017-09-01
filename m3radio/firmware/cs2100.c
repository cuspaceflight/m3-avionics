#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "ch.h"
#include "hal.h"
#include "cs2100.h"
#include "m3radio_status.h"

/* CS2100 I²C address */
#define CS2100_ADDR (0x4E)


/* CS2100 register addresses */
#define CS2100_DEVICE_ID    (0x01)
#define CS2100_DEVICE_CTRL  (0x02)
#define CS2100_DEVICE_CFG_1 (0x03)
#define CS2100_GLOBAL_CFG   (0x05)
#define CS2100_RATIO_1      (0x06)
#define CS2100_RATIO_2      (0x07)
#define CS2100_RATIO_3      (0x08)
#define CS2100_RATIO_4      (0x09)
#define CS2100_FUNCT_CFG_1  (0x16)
#define CS2100_FUNCT_CFG_2  (0x17)
#define CS2100_FUNCT_CFG_3  (0x1E)


/* CS2100 register bits */
#define CS2100_DEVICE_CTRL_UNLOCK           (1<<7)
#define CS2100_DEVICE_CTRL_AUX_OUT_DIS      (1<<1)
#define CS2100_DEVICE_CTRL_CLK_OUT_DIS      (1<<0)
#define CS2100_DEVICE_CFG_1_R_MOD_SEL(x)    ((x<<5) & 0xE0)
#define CS2100_DEVICE_CFG_1_AUX_OUT_SRC_REF_CLK         (0<<1)
#define CS2100_DEVICE_CFG_1_AUX_OUT_SRC_CLK_IN          (1<<1)
#define CS2100_DEVICE_CFG_1_AUX_OUT_SRC_CLK_OUT         (1<<1)
#define CS2100_DEVICE_CFG_1_AUX_OUT_SRC_CLK_PLL_LOCK    (1<<1)
#define CS2100_DEVICE_CFG_1_EN_DEV_CFG_1    (1<<0)
#define CS2100_GLOBAL_CFG_FREEZE            (1<<3)
#define CS2100_GLOBAL_CFG_EN_DEV_CFG_2      (1<<0)
#define CS2100_FUNCT_CFG_1_CLK_SKIP_EN      (1<<7)
#define CS2100_FUNCT_CFG_1_AUX_LOCK_CFG     (1<<6)
#define CS2100_FUNCT_CFG_1_REF_CLK_DIV_4    (0<<3)
#define CS2100_FUNCT_CFG_1_REF_CLK_DIV_2    (1<<3)
#define CS2100_FUNCT_CFG_1_REF_CLK_DIV_1    (2<<3)
#define CS2100_FUNCT_CFG_2_CLK_OUT_UNL      (1<<4)
#define CS2100_FUNCT_CFG_2_L_F_RATIO_CFG    (1<<3)
#define CS2100_FUNCT_CFG_3_CLK_IN_BW_1HZ    (0<<4)
#define CS2100_FUNCT_CFG_3_CLK_IN_BW_2HZ    (1<<4)
#define CS2100_FUNCT_CFG_3_CLK_IN_BW_4HZ    (2<<4)
#define CS2100_FUNCT_CFG_3_CLK_IN_BW_8HZ    (3<<4)
#define CS2100_FUNCT_CFG_3_CLK_IN_BW_16HZ   (4<<4)
#define CS2100_FUNCT_CFG_3_CLK_IN_BW_32HZ   (5<<4)
#define CS2100_FUNCT_CFG_3_CLK_IN_BW_64HZ   (6<<4)
#define CS2100_FUNCT_CFG_3_CLK_IN_BW_128HZ  (7<<4)

static I2CDriver* cs2100_i2cd;
static void cs2100_error(const char* err);
static uint8_t cs2100_read(uint8_t reg_addr);
static bool cs2100_write(uint8_t reg_addr, uint8_t data);


static const I2CConfig i2c_cfg = {
    OPMODE_I2C,
    5250,
    STD_DUTY_CYCLE,
};

static void cs2100_error(const char* err)
{
    /* TODO error handling */
    m3status_set_error(M3RADIO_COMPONENT_PLL,
                       M3RADIO_ERROR_PLL);
    chSysHalt(err);
}


static uint8_t cs2100_read(uint8_t reg_addr)
{
    static uint8_t res __attribute__((section("DATA_RAM")));
    static uint8_t read_reg_addr __attribute__((section("DATA_RAM")));

    read_reg_addr = reg_addr;
    msg_t result = i2cMasterTransmitTimeout(
        cs2100_i2cd, CS2100_ADDR, &read_reg_addr, 1, &res, 1, MS2ST(20));

   if(result == MSG_OK) {
        return res;
    } else if(result == MSG_RESET) {
        i2cflags_t i2c_errs = i2cGetErrors(cs2100_i2cd);
        (void)i2c_errs;
        cs2100_error("I2C write error");
        return 0;
    } else if(result == MSG_TIMEOUT) {
        cs2100_error("I2C write timeout");
        return 0;
    } else {
        return 0;
    }
}


static bool cs2100_write(uint8_t reg_addr, uint8_t data)
{
    static uint8_t buf[2] __attribute__((section("DATA_RAM")));
    buf[0] = reg_addr;
    buf[1] = data;

    msg_t result = i2cMasterTransmitTimeout(
        cs2100_i2cd, CS2100_ADDR, buf, 2, NULL, 0, MS2ST(20));

    if(result == MSG_OK) {
        return true;
    } else if(result == MSG_RESET) {
        i2cflags_t i2c_errs = i2cGetErrors(cs2100_i2cd);
        (void)i2c_errs;
        cs2100_error("I2C write error");
        return false;
    } else if(result == MSG_TIMEOUT) {
        cs2100_error("I2C write timeout");
        return false;
    } else {
        return false;
    }
}


void cs2100_configure(I2CDriver* i2cd)
{
    cs2100_i2cd = i2cd;
    i2cStart(cs2100_i2cd, &i2c_cfg);

    /* Set reference clock divider to /2, to get 26MHz TCXO into range.
     */
    cs2100_write(CS2100_FUNCT_CFG_1, CS2100_FUNCT_CFG_1_REF_CLK_DIV_2);

    /* Don't drive outputs when PLL is unlocked,
     * set ratio to high accuracy.
     */
    cs2100_write(CS2100_FUNCT_CFG_2, CS2100_FUNCT_CFG_2_L_F_RATIO_CFG);

    /* Set the PLL bandwidth after-lock to the minimum, 1Hz.
     * Should roughly match with the GPS.
     */
    cs2100_write(CS2100_FUNCT_CFG_3, CS2100_FUNCT_CFG_3_CLK_IN_BW_1HZ);

#if 1
    /* Set ratio to 26MHz / 1MHz = 26
     * 26 * (1<<20) = 0x01A00000
     * This register is big endian.
     */
    cs2100_write(CS2100_RATIO_1, 0x01);
    cs2100_write(CS2100_RATIO_2, 0xA0);
    cs2100_write(CS2100_RATIO_3, 0x00);
    cs2100_write(CS2100_RATIO_4, 0x00);
#else
    /* Ratio 4MHz / 4MHz = 1
     * 1 * (1<<20) = 0x0010_0000
     */
    cs2100_write(CS2100_RATIO_1, 0x00);
    cs2100_write(CS2100_RATIO_2, 0x10);
    cs2100_write(CS2100_RATIO_3, 0x00);
    cs2100_write(CS2100_RATIO_4, 0x00);
#endif

    /* Don't shift the ratio register at all,
     * output PLL lock on aux (not connected but can be probed),
     * must set EN_DEV_CFG_1 to 1.
     */
    cs2100_write(CS2100_DEVICE_CFG_1,
                 CS2100_DEVICE_CFG_1_R_MOD_SEL(0) |
                 CS2100_DEVICE_CFG_1_AUX_OUT_SRC_CLK_PLL_LOCK |
                 CS2100_DEVICE_CFG_1_EN_DEV_CFG_1);

    /* Must set EN_DEV_CFG_2 to 2. */
    cs2100_write(CS2100_GLOBAL_CFG, CS2100_GLOBAL_CFG_EN_DEV_CFG_2);

    /* Wait for PLL to lock. */
    bool unlocked;
    do {
        static uint8_t ctrl __attribute__((section("DATA_RAM")));
        ctrl = cs2100_read(CS2100_DEVICE_CTRL);
        unlocked = ctrl & CS2100_DEVICE_CTRL_UNLOCK;
        chThdSleepMilliseconds(10);
    } while(unlocked);
}


void cs2100_set_pll(void)
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

    /* Turn off the PLLs */
    RCC->CR &= ~(RCC_CR_PLLON | RCC_CR_PLLI2SON);

    /* Wait for PLLs to be off */
    while(RCC->CR & RCC_CR_PLLRDY);

    /* Set PLL input to HSE, M to HSE/2=13, default N, P, Q. */
    RCC->PLLCFGR = STM32_PLLQ | STM32_PLLP | STM32_PLLN |
                   STM32_PLLSRC_HSE | 13;

    /* Enable PLLs */
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
