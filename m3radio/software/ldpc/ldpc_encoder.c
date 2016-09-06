#include "ldpc_encoder.h"
#include <stdlib.h>
#include <string.h>

/* Parity related constants from CCSDS 213.1-O-1. */
static const uint32_t ldpc_256_128_w[4][4] = {
    {0x73F5E839, 0x0220CE51, 0x36ED68E9, 0xF39EB162},
    {0xBAC812C0, 0xBCD24379, 0x4786D928, 0x5A09095C},
    {0x7DF83F76, 0xA5FF4C38, 0x8E6C0D4E, 0x025EB712},
    {0xBAA37B32, 0x60CB31C5, 0xD0F66A31, 0xFAF511BC}
};

/* Encode 16 bytes of `data` to 32 bytes of `coded`. */
void ldpc_encode(uint8_t* data, uint8_t* coded)
{
    int i, j;

    /* Code is systematic so first 16 bytes are identical. */
    memcpy(coded, data, 16);
    /* Set all parity bits to 0 to initialise. */
    memset(coded+16, 0, 16);

    /* Run through each of 128 parity check equations. */
    for(i=0; i<128; i++) {
        uint8_t id8  = i / 8;
        uint8_t im8  = i % 8;
        uint8_t id32 = i / 32;
        uint8_t im32 = i % 32;
        uint32_t parity = 0;

        /* Run through each of 128 bits of input data. */
        for(j=0; j<128; j++) {
            uint8_t jd8  = j / 8;
            uint8_t jm8  = j % 8;
            uint8_t jd32 = j / 32;
            uint8_t jm32 = j % 32;
            uint8_t pty_bit;

            /* Pick the current bit out of the data buffer. */
            uint8_t data_bit = (data[jd8] >> (7-jm8)) & 1;

            /* Select the hex constant for this data bit (row)
             * and parity check (column).
             */
            uint32_t pty_int = ldpc_256_128_w[jd32][id32];

            /* Rotate the hex constant around to get the right value
             * for our row inside this block. Should compile to ROR.
             */
            pty_int = pty_int >> jm32 | pty_int << (32-jm32);

            /* Pick the appropriate bit out of the parity check. */
            pty_bit = (pty_int >> (31-im32)) & 1;

            /* Tally up this parity check. */
            parity += data_bit & pty_bit;
        }

        /* Store the result of this parity check in the output. */
        coded[16+id8] |= (parity % 2) << (7 - im8);
    }
}
