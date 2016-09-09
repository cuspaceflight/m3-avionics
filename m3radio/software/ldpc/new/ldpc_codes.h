#ifndef LDPC_CODES_H
#define LDPC_CODES_H
#include <stdint.h>

/* Available LDPC codes.
 * n is the block length (bits transmitted over the air),
 * k is the data length (number of user bits encoded).
 */
enum ldpc_code {
    LDPC_CODE_N128_K64,
    LDPC_CODE_N256_K128,
    LDPC_CODE_N512_K256,
    LDPC_CODE_N1280_K1024,
    LDPC_CODE_N1536_K1024,
    LDPC_CODE_N2048_K1024,
};

/* Fill h with the appropriate parity check matrix.
 * Required size of h:
 *  (128, 64)       1024 bytes      256 words       uint32_t[64][4]
 *  (256, 128)      4096 bytes      1024 words      uint32_t[128][8]
 *  (512, 256)      16384 bytes     4096 words      uint32_t[256][16]
 *  (1280, 1024)    67584 bytes     16896 words     uint32_t[384][44]
 *  (1536, 1024)    172032 bytes    43008 words     uint32_t[768][56]
 *  (2048, 1024)    491520 bytes    122880 words    uint32_t[1536][80]
 * Note the larger codes are punctured so the parity check matrix is larger
 * than the usual (n-k, n) size.
 */
void ldpc_codes_init_paritycheck(enum ldpc_code code, uint32_t* h);

/* Gets a pointer to the relevant constants for generator matrices.
 * Also sets n and k (code size) and b (circulant block size).
 * The returned pointer is to a uint32_t[k/b][(n-k)/32],
 * each row has (n-k) bits ((n-k)/32 words),
 * there are k/b rows, which each represent b rows of the actual generator
 * matrix, which has k rows.
 */
uint32_t const * ldpc_codes_get_g(enum ldpc_code code, int* n, int* k, int* b);

#endif
