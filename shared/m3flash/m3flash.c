#include "ch.h"
#include "hal.h"
#include "m3flash.h"
#include "m3status.h"

#define M3FLASH_CFG_SECTOR (11)

static uint32_t m3flash_crc(uint32_t* src, size_t n);
static inline bool m3flash_wait_write(void);

bool m3flash_write(uint32_t* src, uint32_t* dst, size_t n)
{
    chDbgAssert(dst >= (uint32_t*)0x080e0000 && dst < (uint32_t*)0x08100000,
                "dst must be in flash sector 11");
    chDbgAssert((src >= (uint32_t*)0x20000000 && src < (uint32_t*)0x20020000)
                ||
                (src >= (uint32_t*)0x10000000 && src < (uint32_t*)0x10010000),
                "src must be in ram");

    size_t i;
    uint32_t checksum = m3flash_crc(src, n);

    /* Wait for any ongoing flash operations */
    while(FLASH->SR & FLASH_SR_BSY);

    /* Unlock flash */
    FLASH->KEYR = 0x45670123;
    FLASH->KEYR = 0xCDEF89AB;

    /* Erase our configuration sector */
    FLASH->CR = FLASH_CR_SER | (M3FLASH_CFG_SECTOR<<3);
    FLASH->CR |= FLASH_CR_STRT;

    /* Wait for erase completion */
    while(FLASH->SR & FLASH_SR_BSY);

    /* Write flash in 32bit chunks */
    FLASH->CR = FLASH_CR_PG | FLASH_CR_PSIZE_1;
    bool write_ok = true;
    for(i=0; i<n; i++) {
        dst[i] = src[i];
        if(!m3flash_wait_write()) {
            write_ok = false;
            break;
        }
    }

    /* Write checksum */
    dst[n] = checksum;
    m3flash_wait_write();

    /* Re-lock flash */
    FLASH->CR |= FLASH_CR_LOCK;

    return write_ok;
}

bool m3flash_read(uint32_t* src, uint32_t* dst, size_t n) {
    chDbgAssert(src >= (uint32_t*)0x080e0000 && src < (uint32_t*)0x08100000,
                "src must be in flash sector 11");
    chDbgAssert((dst >= (uint32_t*)0x20000000 && dst < (uint32_t*)0x20020000)
                ||
                (dst >= (uint32_t*)0x10000000 && dst < (uint32_t*)0x10010000),
                "dst must be in ram");
    size_t i;
    for(i=0; i<n; i++) {
        dst[i] = src[i];
    }
    uint32_t flash_crc = src[n];
    uint32_t ram_crc = m3flash_crc(dst, n);
    if(ram_crc == flash_crc) {
        return true;
    } else {
        return false;
    }
}

static uint32_t m3flash_crc(uint32_t* src, size_t n) {
    uint32_t crc;
    size_t i;
    RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
    CRC->CR |= CRC_CR_RESET;
    for(i=0; i<n; i++) {
        CRC->DR = src[i];
    }
    crc = CRC->DR;
    RCC->AHB1ENR &= ~RCC_AHB1ENR_CRCEN;
    return crc;
}

static inline bool m3flash_wait_write(void) {
    while(FLASH->SR & FLASH_SR_BSY);
    if(FLASH->SR & FLASH_SR_PGSERR ||
       FLASH->SR & FLASH_SR_PGPERR ||
       FLASH->SR & FLASH_SR_PGAERR ||
       FLASH->SR & FLASH_SR_WRPERR)
    {
        return false;
    } else {
        return true;
    }
}
