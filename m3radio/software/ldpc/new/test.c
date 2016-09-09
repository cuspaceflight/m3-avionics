#include <stdlib.h>
#include <stdio.h>

#include "ldpc_encoder.h"
#include "ldpc_decoder.h"
#include "ldpc_codes.h"

#define n (1408)
#define r (384)
#define m (128)
uint32_t h[r][n/32];

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    ldpc_codes_init_paritycheck(LDPC_CODE_N1280_K1024, (uint32_t*)h);

    printf("P1\n%d %d\n", n, r);

    int i, j;
    for(i=0; i<r; i++) {
        for(j=0; j<n; j++) {
            uint32_t hh = h[i][j/32];
            uint8_t bit = (hh & (1<<(31-(j%32)))) >> (31 - (j%32));
            printf("%d", 1-bit);
        }
        printf("\n");
    }
    printf("\n");

    return 0;
}
