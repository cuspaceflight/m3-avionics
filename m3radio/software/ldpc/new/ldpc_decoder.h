#ifndef LDPC_DECODER_H
#define LDPC_DECODER_H
#include <stdint.h>
#include <stdbool.h>
#include "ldpc_codes.h"

/* Decode received codeword into data using bit flipping algorithm.
 * This algorithm is very quick, uses little memory, and only requires
 * hard information, but is much less capable at decoding than
 * the message passing algorithm.
 * h must have been previously initialised by ldpc_code_init_paritycheck for
 * the specified code,
 * codeword must be n/8 bytes long and each bit is a hard decision,
 * data must be n/8 bytes long and is written with the decoded codeword,
 * working must be n+p bytes long and is used as a scratch working area
 * (the same length as vs in ldpc_codes_init_sparse_paritycheck).
 * Returns true on decoding success, false otherwise.
 */
bool ldpc_decode_bf(enum ldpc_code code, uint32_t* h,
                    const uint8_t* codeword, uint8_t* data, uint8_t* working);

/* Decode LLRs into data using message passing algorithm.
 * This algorithm is slower and ideally requires soft information,
 * but decodes very close to optimal.
 * h must have been previously initialised by ldpc_code_init_paritycheck for
 *   the specified code,
 * llrs must be n floats long, where positive numbers are more likely to be 0,
 * data must be n/8 bytes long.
 * Returns true on decoding success, false otherwise.
 */
bool ldpc_decode_mp(enum ldpc_code code, uint32_t* h,
                    const float* llrs, uint8_t* data);

/* Create approximate LLRs using just the channel SNR and the received data.
 * Can be used to feed the message passing algorithm soft-ish information.
 * codeword must be n/8 bytes long, llrs must be n floats long.
 */
void ldpc_snr_to_llrs(enum ldpc_code code, const uint8_t* codeword,
                      float* llrs, float snr);

#endif
