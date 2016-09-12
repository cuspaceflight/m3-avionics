#include <stdlib.h>
#include <stdio.h>

#include "ldpc_encoder.h"
#include "ldpc_decoder.h"
#include "ldpc_codes.h"

const enum ldpc_code code = LDPC_CODE_N2048_K1024;
const int n = 2048;
const int k = 1024;
uint32_t g[1024][32];

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    int i;
    uint8_t data[128];
    uint8_t code1[256] = {0};
    uint8_t code2[256] = {0};
    (void)code1;
    (void)code2;
    (void)data;

    for(i=0; i<k/8; i++)
        data[i] = i;

    ldpc_codes_init_generator(code, (uint32_t*)g);

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


#if 1
    const int iters = 1000000;
    printf("Running benchmark, %d iterations...\n", iters);
    for(i=0; i<iters; i++)
        ldpc_encode_fast(code, (uint32_t*)g, data, code1);
    printf("Done.\n\n");
#endif

    return 0;
}
