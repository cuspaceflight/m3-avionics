#ifndef LDPC_DECODER_H
#define LDPC_DECODER_H
#include <stdint.h>
#include "ldpc_codes.h"

/* Decode received codeword into data using bit flipping algorithm.
 * This algorithm is very quick, uses little memory, and only requires
 * hard information, but is much less capable at decoding than
 * the message passing algorithm.
 * codeword must be n/8 bytes, data must be n/8 bytes,
 * h must have been previously initialised by ldpc_code_init_paritycheck for
 * the specified code.
 */
void ldpc_decode_bf(uint8_t* codeword, uint8_t* data,
                    enum ldpc_code code, uint32_t* h);

/* Decode LLRs into data using message passing algorithm.
 * This algorithm is slower and ideally requires soft information,
 * but decodes very close to optimal.
 * llrs must be n long, data must be n/8 bytes,
 * h must have been previously initialised by ldpc_code_init_paritycheck for
 * the specified code.
 */
void ldpc_decode_mp(float* llrs, uint8_t* data,
                    enum ldpc_code code, uint32_t* h);

/* Create approximate LLRs using just the channel SNR and the received data.
 * Can be used to feed the message passing algorithm soft-ish information.
 * codeword must be n/8 bytes, llrs must be n long.
 */
void ldpc_snr_to_llrs(uint8_t codeword, float* llrs, float snr);

#endif
