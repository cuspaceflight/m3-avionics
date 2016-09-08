#ifndef LDPC_ENCODER_H
#define LDPC_ENCODER_H
#include <stdint.h>
#include "ldpc_codes.h"

/* Encode data into codeword to be transmitted.
 * data must be k/8 bytes, codeword must be n/8 bytes.
 */
void ldpc_encode(uint8_t* data, uint8_t* codeword, enum ldpc_code code);

#endif
