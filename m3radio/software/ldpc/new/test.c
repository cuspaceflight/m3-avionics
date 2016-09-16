#include <stdlib.h>
#include <stdio.h>

#include "ldpc_encoder.h"
#include "ldpc_decoder.h"
#include "ldpc_codes.h"

const enum ldpc_code code = LDPC_CODE_N1280_K1024;
uint32_t g[1024][32];
uint32_t h[122880];
uint16_t ci[7680], vi[7680], cs[1537], vs[2561];
float llrs[2048];
float workingf[15360];
uint8_t working[2944];
uint8_t data[128];
uint8_t code1[256] = {0};
uint8_t code2[256] = {0};
uint8_t code_out[256];

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    int n=0, k=0, p=0, i, j;
    (void)code1;
    (void)code2;
    (void)data;
    (void)j;
    (void)working;

    ldpc_codes_get_params(code, &n, &k, &p, NULL, NULL, NULL);

    for(i=0; i<k/8; i++)
        data[i] = i;

#if 0
    /* See how many bit errors we can tolerate */
    ldpc_codes_init_paritycheck(code, h);
    ldpc_codes_init_sparse_paritycheck(code, h, ci, cs, vi, vs);

    int biterrors;
    for(biterrors = 0; biterrors < n; biterrors++) {
        for(i=0; i<k/8; i++) {
            data[i] = i;
            code_out[i] = 0;
        }

        ldpc_encode_small(code, data, code1);
        ldpc_decode_ber_to_llrs(code, code1, llrs, (float)biterrors/(float)n);
        /*ldpc_decode_hard_llrs(code, code1, llrs);*/

        for(i=0; i<biterrors; i++) {
            llrs[i] = -llrs[i];
        }

        /*bool result = ldpc_decode_bf(code, ci, cs, cl, code1, code_out, working);*/
        bool result = ldpc_decode_mp(code, ci, cs, vi, vs, llrs, code_out, workingf);

        bool check = true;
        for(i=0; i<k/8; i++) {
            if(code_out[i] != i) check = false;
        }
        printf("%2d errors, decoded: %d, check: %d\n", biterrors, result, check);
        for(i=0; i<(n+p)/8; i++)
            printf("%02X ", code_out[i]);
        printf("\n\n");

        if(!check) {
            printf("BER at failure: %.2f%%\n", 100.0f*(float)biterrors/(float)n);
            break;
        }
    }
#endif

#if 1
    ldpc_codes_init_paritycheck(code, h);
    ldpc_codes_init_sparse_paritycheck(code, h, ci, cs, vi, vs);
    for(i=0; i<k/8; i++) {
        data[i] = i;
        code_out[i] = 0;
    }
    ldpc_encode_small(code, data, code1);
    ldpc_decode_ber_to_llrs(code, code1, llrs, 1.0/50.0);
    for(i=0; i<n/50; i++) {
        llrs[i] = -llrs[i];
    }
    for(i=0; i<100; i++) {
        bool result = ldpc_decode_mp(code, ci, cs, vi, vs, llrs, code_out, workingf);
        bool check = true;
        for(j=0; j<k/8; j++) {
            if(code_out[j] != j) {
                check = false;
                break;
            }
        }
        if(check && result) printf(".");
        else printf("!");
        fflush(stdout);
    }
    printf("\n");
#endif

#if 0
    ldpc_codes_init_generator(code, (uint32_t*)g);
#endif

#if 0
    ldpc_encode_small(code, data, code1);
    ldpc_encode_fast(code, (uint32_t*)g, data, code2);

    for(i=0; i<n/8; i++) {
        printf("%02X %02X\n", code1[i], code2[i]);
    }
    printf("\n");
#endif

#if 0
    int j, k;
    printf("P1\n%d %d\n", 1024, 1024);
    for(i=0; i<1024; i++) {
        for(j=0; j<32; j++) {
            for(k=0; k<32; k++) {
                printf("%d", 1 - ((g[i][j] >> (31-k)) & 1));
            }
        }
        printf("\n");
    }
    printf("\n");
#endif


#if 0
    const int iters = 1000000;
    printf("Running benchmark, %d iterations...\n", iters);
    for(i=0; i<iters; i++)
        ldpc_encode_fast(code, (uint32_t*)g, data, code1);
    printf("Done.\n\n");
#endif

    return 0;
}
