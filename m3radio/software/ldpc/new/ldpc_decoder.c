/*
 * LDPC decoding functions
 * Copyright 2016 Adam Greig
 */

#include <string.h>
#include "ldpc_decoder.h"

#include <stdio.h>

bool ldpc_decode_bf(enum ldpc_code code,
                    uint16_t* ci, uint16_t* cs, uint8_t* cl,
                    const uint8_t* input, uint8_t* output, uint8_t* working)
{
    int n, k, p, i, a, a_idx, max_violations, iters;
    const int max_iters = 20;

    uint8_t * codeword, * violations;

    ldpc_codes_get_params(code, &n, &k, &p, NULL, NULL);

    /* Split up the working area */
    codeword = working;
    violations = working + (n + p)/8;

    /* Copy input to codeword space */
    memcpy(codeword, input, n/8);
    /* Set all the punctured bits to 0 */
    memset(codeword + n/8, 0, p/8);

    /* Run the bit flipping algorithm */
    for(iters=0; iters<max_iters; iters++) {
        /* Clear the violations counters */
        memset(violations, 0, n+p);
        max_violations = 0;

        /* For each parity check, work out the parity sum */
        for(i=0; i<(n-k+p); i++) {
            int parity = 0;
            /* For each bit, update the parity sum */
            for(a_idx=cs[i]; a_idx<cs[i]+cl[i]; a_idx++) {
                a = ci[a_idx];
                parity += (codeword[a/8] >> (7-(a%8))) & 1;
            }

            /* If the check has odd parity, add one violation to each
             * variable node involved in the check */
            if((parity & 1) != 0) {
                for(a_idx=cs[i]; a_idx<cs[i]+cl[i]; a_idx++) {
                    a = ci[a_idx];
                    violations[a]++;
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
            memcpy(output, codeword, k/8);
            return true;
        } else {
            /* Otherwise flip all bits that had the maximum violations */
            for(a=0; a<(n+p); a++) {
                if(violations[a] == max_violations) {
                    codeword[a/8] ^= 1<<(7-(a%8));
                }
            }
        }
    }

    /* If we didn't successfully decode in max_iters, fail here. */
    return false;
}
