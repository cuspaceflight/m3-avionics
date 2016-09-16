#ifndef LDPC_DECODER_H
#define LDPC_DECODER_H
#include <stdint.h>
#include <stdbool.h>
#include "ldpc_codes.h"

/* Decode received input into output using bit flipping algorithm.
 * This algorithm is very quick, uses little memory, and only requires
 * hard information, but is much less capable at decoding than
 * the message passing algorithm.
 *
 * (ci, cs) must all have been initialised by
 * ldpc_codes_init_sparse_paritycheck for the appropriate code.
 * input must be n/8 bytes long and each bit is a hard decision.
 * output must be k/8 bytes long and is written with the decoded data.
 * working must be n/8 + n bytes long and is used as a scratch working area:
 *
 * Code             Length of working area
 * (128, 64)        144
 * (256, 128)       288
 * (512, 256)       576
 *
 * Returns true on decoding success, false otherwise.
 *
 * Note that this algorithm is not suitable to the longer codes which use
 * puncturing, as it cannot represent the unknown bits.
 * It's possible to write an erasure correcting step before the bit flipping
 * step, which should allow this code to work with puncturing, but that work
 * is not done yet.
 */
bool ldpc_decode_bf(enum ldpc_code code,
                    uint16_t* ci, uint16_t* cs,
                    const uint8_t* input, uint8_t* output, uint8_t* working);

/* Decode LLRs into data using message passing algorithm.
 * This algorithm is slower and ideally requires soft information,
 * but decodes very close to optimal. If you don't have soft information
 * but do have the channel BER, you can use ldpc_decode_ber_to_llrs to
 * go from hard information (bytes from a receiver) to soft information.
 * If you don't have that, you can use ldpc_decode_hard_llrs to generate
 * arbitrary LLRs from the hard information.
 *
 * (ci, cs, vi, vs) must all have been initialised by
 * ldpc_codes_init_sparse_paritycheck for the appropriate code.
 * llrs must be n floats long, where positive numbers are more likely to be 0.
 * output must be (n+p)/8 bytes long, of which the first k/8 bytes will be set
 *     to the original transmitted message (then followed by parity bits).
 * working must be 2*s long:
 *
 * Code             Length of output        Length of working area
 * (128, 64)        16                      1024
 * (256, 128)       32                      2048
 * (512, 256)       64                      4096
 * (1280, 1024)     176                     9984
 * (1536, 1024)     224                     11776
 * (2048, 1024)     320                     15360
 *
 * Returns true on decoding success, false otherwise.
 */
bool ldpc_decode_mp(enum ldpc_code code,
                    uint16_t* ci, uint16_t* cs,
                    uint16_t* vi, uint16_t* vs,
                    const float* llrs, uint8_t* output, float* working);

/* Create approximate LLRs using just the channel BER and the received data.
 * Can be used to feed the message passing algorithm soft-ish information.
 * input must be n/8 bytes long, llrs must be n floats long.
 */
void ldpc_decode_ber_to_llrs(enum ldpc_code code, const uint8_t* input,
                             float* llrs, float ber);

/* Create hard LLRs from hard received data.
 * input must be n/8 bytes long, llrs must be n floats long.
 */
void ldpc_decode_hard_llrs(enum ldpc_code code, const uint8_t* input,
                           float* llrs);

#endif
