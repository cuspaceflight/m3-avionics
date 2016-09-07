#include <stdlib.h>
#include <stdio.h>
#include "ldpc.h"

#define n (256)
#define k (128)
#define m (n/8)
uint32_t h[k][n/32];

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    ldpc_init_paritycheck(LDPC_CODE_N256_K128, (uint32_t*)h);

    int i, j;
    for(i=0; i<k; i++) {
        if(i%m==0) {
            for(j=0;j<n+8;j++) printf("-");
            printf("\n");
        }
        for(j=0; j<n; j++) {
            if(j%m==0)
                printf("|");
            uint32_t hh = h[i][j/32];
            uint8_t bit = (hh & (1<<(31-(j%32)))) >> (31 - (j%32));
            if(bit)
                printf("%d", bit);
            else
                printf(" ");
        }
        printf("|\n");
    }
    for(j=0;j<n+8;j++) printf("-");
    printf("\n");

    return 0;
}
