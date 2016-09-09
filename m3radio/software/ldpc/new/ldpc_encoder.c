/*
 * LDPC encoding functions
 * Copyright 2016 Adam Greig
 */

#include <string.h>
#include <stdio.h>
#include "ldpc_encoder.h"

void ldpc_encode(enum ldpc_code code, uint8_t* data, uint8_t* codeword)
{
    int i, j, n, k, r, b;
    uint32_t const * g = ldpc_codes_get_g(code, &n, &k, &b);
    if(g == NULL) {
        return;
    }

    r = n - k;

    /* Copy data into first part of codeword and initialise second part to 0 */
    memcpy(codeword, data, k/8);
    memset(codeword+k/8, 0x00, r/8);

    printf("ldpc_encode n=%d k=%d r=%d b=%d\n", n, k, r, b);

    /* For each parity check equation */
    for(i=0; i<r; i++) {
        /* Store the current parity */
        uint32_t parity = 0;

        /* For each input data bit */
        for(j=0; j<k; j++) {
            uint32_t h_word;
            uint8_t h_bit;
            uint8_t d_bit;
            int block_start, block_idx, word_start, word_idx;

            block_start = (i/b)*b;
            block_idx = ((i%b) - (j%b)) % b;
            if(block_idx < 0)
                block_idx += b;
            word_start = (block_start + block_idx) / 32;
            word_idx = (block_start + block_idx) % 32;

            /*printf("n=%d k=%d r=%d b=%d i=%d j=%d bs=%d bi=%d ws=%d wi=%d\n",*/
                   /*n, k, r, b, i, j,*/
                   /*block_start, block_idx, word_start, word_idx);*/

            /* Pick our data bit */
            d_bit = (data[j/8] >> (7-(j%8))) & 1;

            /* Pick the packed generator constant for this data bit (row)
             * and parity check (column).
             * We skip (j/b) rows of (r/32) words above us,
             * then (i%b)/32 columns of blocks left of us,
             * then ror/32 words to get to the word with our current bit.
             */
            h_word = g[(j/b)*(r/32) + word_start];

            /* Pick our parity bit */
            h_bit = (h_word >> (31 - (word_idx%32))) & 1;

            /* Add on to the parity check */
            parity += d_bit & h_bit;
        }

        /* Store the result of the parity check */
        codeword[(k/8) + (i/8)] |= (parity & 1) << (7 - (i%8));
    }
    printf("\n");
}
