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

/* Get parameters corresponding to a given code.
 * n: code length (block length; number of transmitted bits)
 * k: code dimension (information length; number of user bits)
 * p: punctured checks (number of parity bits not transmitted)
 * m: sub-matrix size (used in code definition)
 * b: circulant block size (used in generator construction)
 * Specify NULL if you don't care about a certain parameter.
 */
void ldpc_codes_get_params(enum ldpc_code code,
                           int* n, int* k, int* p, int* m, int* b);

/* Fill h with the appropriate parity check matrix, densely packed.
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

/* Fill sparse representations of parity check matrix.
 * This representation has two 1d-lists, ci and vi, one representing the
 * non-zero indices along each row (check nodes), and the other along each
 * column (variable nodes). They allow iterating through the parity matrix
 * connections either from row to column (check to variable node) or vice
 * versa very efficiently.
 *
 * To index into the lists, each has a list of starting points cs and vs, and a
 * list of lengths (node degrees) cl and vl. The length of cs and cl is equal
 * to the number of parity check equations (before puncturing), while the
 * length of vs and vl is equal to the number of variable nodes (or the
 * codeword length, n).
 *
 * All of these must have been allocated before calling this function,
 * with the following lengths:
 * Code             ci, vi      cs, cl      vs, vl
 * (128, 64)         512          64         128
 * (256, 128)       1024         128         256
 * (512, 256)       2048         256         512
 * (1280, 1024)     4992         384        1408
 * (1536, 1024)     5888         768        1792
 * (2048, 1024)     7680        1536        2560
 *
 * The non-sparse parity check matrix h must have been initialised before
 * calling this function, via ldpc_codes_init_paritycheck.
 */
void ldpc_codes_init_sparse_paritycheck(enum ldpc_code code, uint32_t* h,
                                        uint16_t* ci, uint16_t* cs,
                                        uint8_t* cl,
                                        uint16_t* vi, uint16_t* vs,
                                        uint8_t* vl);

/* Gets a pointer to the relevant constants for compact generator matrices.
 * Also sets n and k (code size) and b (circulant block size).
 * The returned pointer is to a uint32_t[k/b][(n-k)/32],
 * each row has (n-k) bits ((n-k)/32 words),
 * there are k/b rows, which each represent b rows of the actual generator
 * matrix, which has k rows.
 */
const uint32_t * ldpc_codes_get_compact_generator(enum ldpc_code code,
                                                  int* n, int* k, int* b);

/* Initialise a generator matrix expanded from the compact circulant form.
 * This allows quicker encoding at the cost of more memory usage.
 * Note this will only initialise the parity part of G, and not the identity
 * matrix, since all supported codes are systematic. This matches what's
 * expected by the non-compact encoder.
 * Required size of g:
 *  (128, 64)       512 bytes       128 words       uint32_t[64][2]
 *  (256, 128)      2048 bytes      512 words       uint32_t[128][4]
 *  (512, 256)      8192 bytes      2048 words      uint32_t[256][8]
 *  (1280, 1024)    32768 bytes     8192 words      uint32_t[1024][8]
 *  (1536, 1024)    65536 bytes     16384 words     uint32_t[1024][16]
 *  (2048, 1024)    131072 bytes    32768 words     uint32_t[1024][32]
 */
void ldpc_codes_init_generator(enum ldpc_code code, uint32_t* g);

#endif
