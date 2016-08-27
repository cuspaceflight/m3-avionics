#ifndef M3FC_FLASH_H
#define M3FC_FLASH_H

/* Write n 32bit words from src (in RAM) to dst (in Flash).
 * Writes a CRC32 checksum after the data for later verification.
 */
void m3fc_flash_write(uint32_t* src, uint32_t* dst, size_t n);

/* Read n 32bit words from src (in Flash) to dst (in RAM).
 * Also reads a CRC32 checksum after the data to verify, and returns true
 * on successful verification.
 */
bool m3fc_flash_read(uint32_t* src, uint32_t* dst, size_t n);

#endif
