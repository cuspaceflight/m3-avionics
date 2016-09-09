#include <stdlib.h>
#include <stdio.h>

#include "ldpc_encoder.h"
#include "ldpc_decoder.h"
#include "ldpc_codes.h"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    int i;
    uint8_t data[128];
    uint8_t code[256] = {0};

    for(i=0; i<128; i++)
        data[i] = i;

    printf("Running benchmark, 1000 iterations of (2048,1024) code...\n");
    for(i=0; i<1000; i++)
        ldpc_encode(LDPC_CODE_N2048_K1024, data, code);
    printf("Done.\n\n");

    return 0;
}
