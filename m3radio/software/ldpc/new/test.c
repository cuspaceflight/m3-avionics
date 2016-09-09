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

    ldpc_encode(LDPC_CODE_N2048_K1024, data, code);
    for(i=0; i<256; i++)
        printf("%02X ", code[i]);
    printf("\n\n");

    ldpc_encode(LDPC_CODE_N1536_K1024, data, code);
    for(i=0; i<192; i++)
        printf("%02X ", code[i]);
    printf("\n\n");

    ldpc_encode(LDPC_CODE_N1280_K1024, data, code);
    for(i=0; i<160; i++)
        printf("%02X ", code[i]);
    printf("\n\n");

    return 0;
}
