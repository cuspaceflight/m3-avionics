/*
 * LDPC encoding functions
 * Copyright 2016 Adam Greig
 */

#include <string.h>
#include "ldpc_encoder.h"
#include <stdio.h>

void ldpc_encode_small(enum ldpc_code code,
                       const uint8_t* data, uint8_t* codeword)
{
    int i, j, n, k, r, b, divb, modb;
    uint32_t const * g = ldpc_codes_get_compact_generator(code, &n, &k, &b);
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

            /* Pick our data bit */
            d_bit = (data[j/8] >> (7-(j%8))) & 1;

            /* If the data is zero, no need to proceed, skip this loop */
            if(d_bit == 0) {
                continue;
            }

            /* Compute the offset to i to look at to find the equivalent
             * of the block we're in rotated right by j.
             */
            io = j&modb;
            if(io > (i&modb)) {
                io -= b;
            }

            /* Pick the packed generator constant for this data bit (row)
             * and parity check (column).
             * We skip (j/b) rows of (r/32) words above us,
             * then (i-io)/32 words to get to the one we want.
             */
            h_word = g[(j>>divb)*(r/32) + (i-io)/32];

            /* Pick our parity bit */
            h_bit = (h_word >> (31 - ((i-io)%32))) & 1;

            /* Add on to the parity check */
            parity += h_bit;
        }

        /* Store the result of the parity check */
        codeword[(k/8) + (i/8)] |= (parity & 1) << (7 - (i%8));
    }
}

void ldpc_encode_fast(enum ldpc_code code, const uint32_t* g,
                      const uint8_t* data, uint8_t* codeword)
{
    int i, j, n, k, r, b;
    (void)ldpc_codes_get_compact_generator(code, &n, &k, &b);
    if(g == NULL) {
        return;
    }

    r = n - k;

    /* Copy data into systematic part of codeword, zero parity part */
    memcpy(codeword, data, k/8);
    memset(codeword+k/8, 0x00, r/8);

    /* For each data bit */
    for(i=0; i<k; i++) {
        uint8_t dbit = (data[i/8] >> (7 - (i%8))) & 1;
        if(dbit) {
            /* For each word of generator matrix */
            for(j=0; j<r/32; j++) {
                /* XOR with the row of g */
                const uint32_t * word = &g[i*(r/32) + j];
                uint32_t * code = (uint32_t*)&codeword[k/8 + j*4];
                *code ^= *word;
            }
        }
    }
}
