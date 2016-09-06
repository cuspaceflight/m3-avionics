#ifndef LDPC_ENCODER_H
#define LDPC_ENCODER_H

#include <stdint.h>

/* Encode 16 bytes of `data` to 32 bytes of `coded`. */
void ldpc_encode(uint8_t* data, uint8_t* coded);

#endif /* LDPC_ENCODER_H */
