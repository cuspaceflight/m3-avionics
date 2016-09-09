/*
 * LDPC encoding functions
 * Copyright 2016 Adam Greig
 */

#include <string.h>
#include "ldpc_encoder.h"

void ldpc_encode(enum ldpc_code code, uint8_t* data, uint8_t* codeword)
{
    int i, j, n, k, r, b, divb, modb;
    uint32_t const * g = ldpc_codes_get_g(code, &n, &k, &b);
    if(g == NULL) {
        return;
    }

    r = n - k;

    /* Silly hand optimisation because compiler can't know b is a power of 2.
     * modb is the bits so (x & modb) == (x % b) for x>0.
     * divb is log2(b) such that (x>>divb) == (x/b)
     */
    modb = b - 1;
    divb = 0;
    while(!((b>>divb)&1)) divb++;

    /* Copy data into first part of codeword and initialise second part to 0 */
    memcpy(codeword, data, k/8);
    memset(codeword+k/8, 0x00, r/8);

    /* For each parity check equation */
    for(i=0; i<r; i++) {
        /* Store the current parity */
        uint32_t parity = 0;

        /* For each input data bit */
        for(j=0; j<k; j++) {
            uint32_t h_word;
            uint8_t h_bit;
            uint8_t d_bit;
            int io;

            /* Compute the offset to i to look at to find the equivalent
             * of the block we're in rotated right by j.
             */
            io = j&modb;
            if(io > (i&modb)) {
                io -= b;
            }

            /* Pick our data bit */
            d_bit = (data[j/8] >> (7-(j%8))) & 1;

            /* Pick the packed generator constant for this data bit (row)
             * and parity check (column).
             * We skip (j/b) rows of (r/32) words above us,
             * then (i-io)/32 words to get to the one we want.
             */
            h_word = g[(j>>divb)*(r/32) + (i-io)/32];

            /* Pick our parity bit */
            h_bit = (h_word >> (31 - ((i-io)%32))) & 1;

            /* Add on to the parity check */
            parity += d_bit & h_bit;
        }

        /* Store the result of the parity check */
        codeword[(k/8) + (i/8)] |= (parity & 1) << (7 - (i%8));
    }
}
