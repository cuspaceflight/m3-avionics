#ifndef LDPC_DECODER_H
#define LDPC_DECODER_H

#include <stdint.h>
#include <stdbool.h>

/* Decode 256 LLRs into 32 bytes of codeword in `coded`. */
bool ldpc_decode(double* llrs, uint8_t* coded);

/* Decode 32 bytes from channel into 32 bytes of codeword */
bool ldpc_hard_decode(uint8_t in[32], uint8_t out[32]);

#endif /* LDPC_DECODER_H */
