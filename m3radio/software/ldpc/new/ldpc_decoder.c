/*
 * LDPC decoding functions
 * Copyright 2016 Adam Greig
 */

#include <string.h>
#include "ldpc_decoder.h"

#include <stdio.h>

bool ldpc_decode_bf(enum ldpc_code code, uint32_t* h,
                    const uint8_t* codeword, uint8_t* data, uint8_t* working)
{
    int n, k, p, i, a, max_violations, iters;
    const int max_iters = 100;
    uint8_t* violations = working;

    ldpc_codes_get_params(code, &n, &k, &p, NULL, NULL);

    /* Copy codeword to data */
    memcpy(data, codeword, n/8);

    for(iters=0; iters<max_iters; iters++) {
        /* Clear the violations counters */
        memset(violations, 0, n+p);
        max_violations = 0;

        /* For each parity check */
        for(i=0; i<(n-k+p); i++) {
            int parity = 0;
            /* For each bit */
            for(a=0; a<(n+p); a++) {
                if(((h[i*(n+p)/32 + a/32] >> (31-(a%32)))&1) == 0) {
                    continue;
                }
                parity += (data[a/8] >> (7-(a%8))) & 1;
            }
            if((parity & 1) != 0) {
                for(a=0; a<(n+p); a++) {
                    if(((h[i*(n+p)/32 + a/32] >> (31-(a%32)))&1) == 1) {
                        violations[a]++;
                    }
                }
            }
        }

        /* Find the maximum number of violations */
        for(a=0; a<(n+p); a++) {
            if(violations[a] > max_violations) {
                max_violations = violations[a];
            }
        }

        if(max_violations == 0) {
            /* No violations means valid codeword */
            return true;
        } else {
            /* Otherwise flip all bits that had the maximum violations */
            for(a=0; a<(n+p); a++) {
                if(violations[a] == max_violations) {
                    data[a/8] ^= 1<<(7-(a%8));
                }
            }
        }
    }

    /* If we didn't successfully decode in max_iters, fail here. */
    return false;
}
