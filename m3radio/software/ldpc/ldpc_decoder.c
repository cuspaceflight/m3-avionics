#include "ldpc_decoder.h"
#include "ldpc_parity_check.h"
#include "ldpc_parity_check_packed.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Use the compact int64_t parity check matrix? */
#define USE_COMPACT_H 0

/* Choose check node update algoritm. */
#define USE_TANH 1
#define USE_TANH_APPROX 0
#define USE_MINSUM 0

/* See if i and a are connected. */
static inline bool ldpc_h(int i, int a)
{
#if USE_COMPACT_H
    return (ldpc_parity_p[i][a / 64] >> (63 - (a % 64))) & 1;
#else
    return ldpc_parity[i][a];
#endif

}

/* Check if x.H=0 implying that we have finished. */
static bool ldpc_check_if_codeword(uint8_t* coded)
{
    int i, a;
    /* For each parity check equation */
    for(i=0; i<128; i++) {
        int parity = 0;

        /* For each coded bit */
        for(a=0; a<256; a++) {
            if(!ldpc_h(i, a)) continue;
            parity += (coded[a/8] >> (7 - (a%8))) & 1;
        }

        /* Return false at the first violated parity check. */
        if(parity % 2 != 0)
            return false;
    }

    return true;
}

static inline double sign(double x)
{
    return (x > 0) - (x < 0);
}

static inline double minstar(double a, double b)
{
    return sign(a) * sign(b) * fmin(fabs(a), fabs(b))
            + log(1.0f + exp(-(fabs(a) + fabs(b))))
            - log(1.0f + exp(-fabs(fabs(a) - fabs(b))))
        ;
}

static inline double phi(double x)
{
    return log((exp(x)+1)/(exp(x)-1));
}

/* Decode 256 LLRs into 32 bytes of codeword in `coded`. */
bool ldpc_decode(double* r, uint8_t* coded)
{
    int iter, a, b, i, j;
    double **u, **v, *llrs;
    const int max_iters = 100;

    /* Check if we can return early. */
    memset(coded, 0, 32);
    for(a=0; a<256; a++) {
        coded[a/8] |= (r[a] < 0.0f) << (7 - (a % 8));
    }
    if(ldpc_check_if_codeword(coded)) {
        /*printf("Codeword already valid, returning.\n");*/
        return true;
    }

    /* Allocate memory for u i->a */
    u = (double**)malloc(128 * sizeof(double*));
    u[0] = (double*)malloc(128 * 256 * sizeof(double));
    for(i=1; i<128; i++) {
        u[i] = u[0] + i * 256;
    }

    /* Allocate memory for v a->i */
    v = (double**)malloc(256 * sizeof(double*));
    v[0] = (double*)malloc(256 * 128 * sizeof(double));
    for(a=1; a<256; a++) {
        v[a] = v[0] + a * 128;
    }

    /* Allocate memory for the LLRs */
    llrs = (double*)malloc(256 * sizeof(double));

    /* Initialisation */
    for(a=0; a<256; a++) {
        llrs[a] = r[a];
        for(i=0; i<128; i++) {
            if(!ldpc_h(i, a)) continue;
            v[a][i] = r[a];
        }
    }

    /*printf("[00] LLR[0]=%f\n", llrs[0]);*/

    /* Cap max iterations. */
    for(iter=0; iter<max_iters; iter++) {
        /* Check nodes to variable nodes */
        for(i=0; i<128; i++) {
            for(a=0; a<256; a++) {
                if(!ldpc_h(i, a)) continue;
#if USE_TANH
                u[i][a] = 1.0f;
                for(b=0; b<256; b++) {
                    if(!ldpc_h(i, b) || b==a) continue;
                    u[i][a] *= tanh(v[b][i] / 2.0f);
                }
                u[i][a] = 2.0f * atanh(u[i][a]);
#endif
#if USE_TANH_APPROX
                u[i][a] = 0.0f;
                double prodsign = 1.0f;
                for(b=0; b<256; b++) {
                    if(!ldpc_h(i, b) || b==a) continue;
                    u[i][a] += phi(fabs(v[b][i]));
                    prodsign *= sign(v[b][i]);
                }
                u[i][a] = prodsign * phi(u[i][a]);
#endif
#if USE_MINSUM
                u[i][a] = 9999.9f;
                for(b=0; b<256; b++) {
                    if(!ldpc_h(i, b) || b==a) continue;
                    u[i][a] = sign(u[i][a]) * sign(v[b][i]) * fmin(fabs(u[i][a]), fabs(v[b][i]));
                    /*u[i][a] = minstar(u[i][a], v[b][i]);*/
                }
#endif
            }
        }

        /* Check if we're done. */
        memset(coded, 0, 32);
        for(a=0; a<256; a++) {
            llrs[a] = r[a];
            for(i=0; i<128; i++) {
                llrs[a] += u[i][a];
            }
            coded[a/8] |= (llrs[a] <= 0.0) << (7 - (a % 8));
        }
        /*printf("[%02d] LLR[0]=%f\n", iter, llrs[0]);*/
        if(ldpc_check_if_codeword(coded)) {
            /*printf("Codeword found after %d iters, returning.\n", iter);*/
            break;
        }

        /* Variable nodes to check nodes */
        for(a=0; a<256; a++) {
            for(i=0; i<128; i++) {
                if(!ldpc_h(i, a)) continue;
                v[a][i] = r[a];
                for(j=0; j<128; j++) {
                    if(!ldpc_h(j, a) || j==i) continue;
                    v[a][i] += u[j][a];
                }
            }
        }

    }

    free(v[0]);
    free(v);
    free(u[0]);
    free(u);
    free(llrs);

    if(iter == max_iters) {
        /*printf("Max iterations exceeded, returning.\n");*/
        return false;
    }

    return true;
}

bool ldpc_hard_decode(uint8_t in[32], uint8_t out[32]) {
    int i, a;
    uint8_t bits[256];
    int violations[256], max_violations;
    int iters;
    const int max_iters = 100;

    /* copy input to bits */
    for(a=0; a<256; a++) {
        bits[a] = (in[a/8] >> (7-(a%8))) & 1;
    }

    for(iters=0; iters<max_iters; iters++) {

        /* clear violations */
        memset(violations, 0, sizeof(violations));
        max_violations = 0;

        /* for each parity check */
        for(i=0; i<128; i++) {
            int parity = 0;
            /* for each bit */
            for(a=0; a<256; a++) {
                if(!ldpc_h(i, a)) continue;
                parity += bits[a];
            }
            /* if check is failed, increment violation count for all bits
             * involved in it */
            if(parity % 2 != 0) {
                for(a=0; a<256; a++) {
                    if(ldpc_h(i, a))
                        violations[a]++;
                }
            }
        }

        /* find maximum number of violations */
        for(a=0; a<256; a++) {
            if(violations[a] > max_violations) {
                max_violations = violations[a];
            }
        }

        /* if no violations, we've cracked the code */
        if(max_violations == 0) {
            memset(out, 0, 32);
            for(a=0; a<256; a++) {
                out[a/8] |= bits[a] << (7 - (a%8));
            }
            return true;
        }

        /* flip all bits with the maximum number of violations */
        for(a=0; a<256; a++) {
            if(violations[a] == max_violations) {
                bits[a] ^= 1;
            }
        }
    }

    return false;
}
