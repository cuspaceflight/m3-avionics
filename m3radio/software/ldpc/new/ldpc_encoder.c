/*
 * LDPC encoding functions
 * Copyright 2016 Adam Greig
 */

#include <string.h>
#include "ldpc_encoder.h"

void ldpc_encode(uint8_t* data, uint8_t* codeword, enum ldpc_code code)
{
    int i, j, n, k, r, m;
    uint32_t const * g = ldpc_codes_get_g(code, &n, &k, &m);
    if(g == NULL) {
        return;
    }

    r = n - k;

    /* Copy data into first part of codeword and initialise second part to 0 */
    memcpy(codeword, data, k/8);
    memset(codeword+k/8, 0x00, r/8);

    /* For each parity check equation */
    for(i=0; i<r; i++) {
        /* Compute a bunch numbers we'll need for indexing stuff.
         */
        uint8_t id8  = i / 8;
        uint8_t im8  = i % 8;
        uint8_t id32 = i / 32;
        uint8_t im32 = i % 32;
        uint32_t parity = 0;

        /* For each input data bit */
        for(j=0; j<k; j++) {
            uint8_t jd8  = j / 8;
            uint8_t jm8  = j % 8;
            uint8_t jd32 = j / 32;
            uint8_t jm32 = j % 32;
            uint8_t parity_bit;

            /* Pick our data bit */
            uint8_t data_bit = (data[jd8] >> (7-jm8)) & 1;

            /* XXX
             * Each block might pack into one or more or a non-integer number
             * of 32 bit blocks. Be careful.
             */

            /* Pick the packed generator constant for this data bit (row)
             * and parity check (column).
             */
            uint32_t gen = g[jd32*(r/32) + id32];

            /* Rotate the constant around to the right value for our row inside
             * this block. Hopefully compiles to a ROR operation.
             */
            gen = gen >> jm32 | gen << (32-jm32);

            /* Pick our parity bit */
            parity_bit = (gen >> (31 - im32)) & 1;

            /* Add on to the parity check */
            parity += data_bit & parity_bit;
        }

        /* Store the result of the parity check */
        codeword[16 + id8] |= (parity & 1) << (7 - im8);
    }
}
