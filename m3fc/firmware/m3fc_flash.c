#include "ch.h"
#include "hal.h"
#include "m3fc_flash.h"
#include "m3fc_status.h"

#define M3FC_FLASH_CFG_SECTOR (11)

static uint32_t m3fc_flash_crc(uint32_t* src, size_t n);
static inline bool m3fc_flash_wait_write(void);

void m3fc_flash_write(uint32_t* src, uint32_t* dst, size_t n)
{
    chDbgAssert(dst >= (uint32_t*)0x080d9400 && dst < (uint32_t*)0x08100000,
                "dst must be in flash");
    chDbgAssert((src >= (uint32_t*)0x20000000 && src < (uint32_t*)0x20020000)
                ||
                (src >= (uint32_t*)0x10000000 && src < (uint32_t*)0x10010000),
                "src must be in ram");

    uint32_t checksum = m3fc_flash_crc(src, n);

    /* Wait for any ongoing flash operations */
    while(FLASH->SR & FLASH_SR_BSY);

    /* Unlock flash */
    FLASH->KEYR = 0x45670123;
    FLASH->KEYR = 0xCDEF89AB;

    /* Configure for 32 bit access */
    FLASH->CR |= FLASH_CR_PSIZE_1;

    /* Erase our configuration sector */
    FLASH->CR |= FLASH_CR_SER | (M3FC_FLASH_CFG_SECTOR<<3);
    FLASH->CR |= FLASH_CR_STRT;

    /* Wait for erase completion */
    while(FLASH->SR & FLASH_SR_BSY);

    /* Clear SER */
    FLASH->CR &= ~FLASH_CR_SER;

    /* Write flash */
    FLASH->CR |= FLASH_CR_PG;
    while(n--) {
        *dst++ = *src++;
        if(!m3fc_flash_wait_write()) {
            break;
        }
    }

    /* Write checksum */
    *dst = checksum;
    m3fc_flash_wait_write();

    /* Re-lock flash */
    FLASH->CR |= FLASH_CR_LOCK;
}

bool m3fc_flash_read(uint32_t* src, uint32_t* dst, size_t n) {
    chDbgAssert(src >= (uint32_t*)0x080d9400 && src < (uint32_t*)0x08100000,
                "src must be in flash");
    chDbgAssert((dst >= (uint32_t*)0x20000000 && dst < (uint32_t*)0x20020000)
                ||
                (dst >= (uint32_t*)0x10000000 && dst < (uint32_t*)0x10010000),
                "dst must be in ram");
    size_t i;
    uint32_t flash_crc = m3fc_flash_crc(src, n);
    for(i=0; i<n; i++) {
        *(dst + i) = *(src + i);
    }
    uint32_t ram_crc = m3fc_flash_crc(dst, n);
    if(ram_crc == flash_crc) {
        return true;
    } else {
        m3status_set_error(M3FC_COMPONENT_FLASH, M3FC_ERROR_FLASH_CRC);
        return false;
    }
}

static uint32_t m3fc_flash_crc(uint32_t* src, size_t n) {
    uint32_t crc;
    RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
    CRC->CR |= CRC_CR_RESET;
    while(n--) {
        CRC->DR = *src++;
    }
    crc = CRC->DR;
    RCC->AHB1ENR &= ~RCC_AHB1ENR_CRCEN;
    return crc;
}

static inline bool m3fc_flash_wait_write(void) {
    while(FLASH->SR & FLASH_SR_BSY);
    if(FLASH->SR & FLASH_SR_PGSERR ||
       FLASH->SR & FLASH_SR_PGPERR ||
       FLASH->SR & FLASH_SR_PGAERR ||
       FLASH->SR & FLASH_SR_WRPERR)
    {
        m3status_set_error(M3FC_COMPONENT_FLASH, M3FC_ERROR_FLASH_WRITE);
        return false;
    } else {
        return true;
    }
}
