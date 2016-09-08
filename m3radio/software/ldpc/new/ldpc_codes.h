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

/* Fill h (which must point to ((n-k)*n)/8 bytes of memory) with the
 * appropriate parity check matrix.
 * h must be at least (n*n - k*n)/8 bytes large.
 */
void ldpc_codes_init_paritycheck(enum ldpc_code code, uint32_t* h);

/* Gets a pointer to the relevant constants for generator matrices.
 * Also sets n and k (code size) and m (circulant block size).
 */
uint32_t const * ldpc_codes_get_g(enum ldpc_code code, int* n, int* k, int* m);

#endif
