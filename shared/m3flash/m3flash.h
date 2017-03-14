#ifndef M3FLASH_H
#define M3FLASH_H

/* Write n 32bit words from src (in RAM) to dst (in Flash).
 * Writes a CRC32 checksum after the data for later verification.
 * Returns true on success and false on failure.
 */
bool m3flash_write(uint32_t* src, uint32_t* dst, size_t n);

/* Read n 32bit words from src (in Flash) to dst (in RAM).
 * Also reads a CRC32 checksum after the data to verify, and returns true
 * on successful verification or false if the CRC did not match.
 */
bool m3flash_read(uint32_t* src, uint32_t* dst, size_t n);

#endif
