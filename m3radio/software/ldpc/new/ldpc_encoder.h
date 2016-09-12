#ifndef LDPC_ENCODER_H
#define LDPC_ENCODER_H
#include <stdint.h>
#include "ldpc_codes.h"

/* Encode data into codeword to be transmitted.
 * This uses the small constants but takes more time.
 * data must be k/8 bytes, codeword must be n/8 bytes.
 */
void ldpc_encode_small(enum ldpc_code code,
                       const uint8_t* data, uint8_t* codeword);

/* Encode data into codeword to be transmitted.
 * This is quick but requires a large in-memory generator matrix.
 * g must have been initialised via ldpc_codes_init_generator().
 * data must be k/8 bytes, codeword must be n/8 bytes.
 */
void ldpc_encode_fast(enum ldpc_code code, const uint32_t* g,
                      const uint8_t* data, uint8_t* codeword);

#endif
