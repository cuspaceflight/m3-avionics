/*
 * LDPC encoding/decoding functions
 * Copyright 2016 Adam Greig
 */

#include <string.h>
#include <stdio.h>
#include "ldpc.h"

/*
 * Generator matrix constants for the three CCSDS 231.1-O-1 codes,
 * in compact circulant form. Rows 1, M+1, 2M+1, 3M+1 are given, with
 * intermediate rows found by right circular shifts of these rows.
 */
static const uint32_t g_n128_k64[4*2] = {
    0x0E69166B, 0xEF4C0BC2, 0x7766137E, 0xBB248418,
    0xC480FEB9, 0xCD53A713, 0x4EAA22FA, 0x465EEA11,
};
static const uint32_t g_n256_k128[4*4] = {
    0x73F5E839, 0x0220CE51, 0x36ED68E9, 0xF39EB162,
    0xBAC812C0, 0xBCD24379, 0x4786D928, 0x5A09095C,
    0x7DF83F76, 0xA5FF4C38, 0x8E6C0D4E, 0x025EB712,
    0xBAA37B32, 0x60CB31C5, 0xD0F66A31, 0xFAF511BC,
};
static const uint32_t g_n512_k256[4*8] = {
    0x1D21794A, 0x22761FAE, 0x59945014, 0x257E130D,
    0x74D60540, 0x03794014, 0x2DADEB9C, 0xA25EF12E,
    0x60E0B662, 0x3C5CE512, 0x4D2C81EC, 0xC7F469AB,
    0x20678DBF, 0xB7523ECE, 0x2B54B906, 0xA9DBE98C,
    0xF6739BCF, 0x54273E77, 0x167BDA12, 0x0C6C4774,
    0x4C071EFF, 0x5E32A759, 0x3138670C, 0x095C39B5,
    0x28706BD0, 0x45300258, 0x2DAB85F0, 0x5B9201D0,
    0x8DFDEE2D, 0x9D84CA88, 0xB371FAE6, 0x3A4EB07E,
};


/*
 * Parity check matrices corresponding to the above codes.
 * This representation mirrors the definition in CCSDS 231.1-O-1,
 * and can be expanded at runtime to create the actual matrix in memory.
 * The macros each relate to a single MxM sub-block, where M=n/8.
 */
#define HZ          (0 << 6)                /* All-zero sub-matrix           */
#define HI          (1 << 6)                /* Identity matrix sub-matrix    */
#define HP          (2 << 6)                /* nth right circular shift of I */
#define HS          (HI | HP)               /* HI + HP(n)                    */

static const uint8_t h_n128_k64[4][8] = {
    {HS| 7, HP| 2, HP|14, HP| 6, HZ   , HP| 0, HP|13, HI   },
    {HP| 6, HS|15, HP| 0, HP| 1, HI   , HZ   , HP| 0, HP| 7},
    {HP| 4, HP| 1, HS|15, HP|14, HP|11, HI   , HZ   , HP| 3},
    {HP| 0, HP| 1, HP| 9, HS|13, HP|14, HP| 1, HI   , HZ   },
};
static const uint8_t h_n256_k128[4][8] = {
    {HS|31, HP|15, HP|25, HP| 0, HZ   , HP|20, HP|12, HI   },
    {HP|28, HS|30, HP|29, HP|24, HI   , HZ   , HP| 1, HP|20},
    {HP| 8, HP| 0, HS|28, HI| 1, HP|29, HI   , HZ   , HP|21},
    {HP|18, HP|30, HP| 0, HS|30, HP|25, HP|26, HI   , HZ   },
};
static const uint8_t h_n512_k256[4][8] = {
    {HS|63, HP|30, HP|50, HP|25, HZ   , HP|43, HP|62, HI   },
    {HP|56, HS|61, HP|50, HP|23, HI   , HZ   , HP|37, HP|26},
    {HP|16, HP| 0, HS|55, HP|27, HP|56, HI   , HZ   , HP|43},
    {HP|35, HP|56, HP|62, HS|11, HP|58, HP| 3, HI   , HZ   },
};

void ldpc_encode(uint8_t* data, uint8_t* codeword, enum ldpc_code code)
{
    int i, j, n, k, r;
    uint32_t const * g;

    switch(code) {
        case LDPC_CODE_N128_K64:
            n = 128;
            k = 64;
            g = g_n128_k64;
            break;

        case LDPC_CODE_N256_K128:
            n = 256;
            k = 128;
            g = g_n256_k128;
            break;

        case LDPC_CODE_N512_K256:
            n = 512;
            k = 256;
            g = g_n512_k256;
            break;

        default:
            return;
    }

    r = n - k;

    /* Copy data into first part of codeword and initialise second part to 0 */
    memcpy(codeword, data, 16);
    memset(codeword+16, 0x00, 16);

    /* For each parity check equation */
    for(i=0; i<r; i++) {
        /* Compute a bunch numbers we'll need for indexing stuff.
         */
        uint8_t id8  = i / 8;
        uint8_t im8  = i % 8;
        uint8_t id32 = i / 32;
        uint8_t im32 = i % 32;
        uint32_t parity = 0;

        /* For each input data bit */
        for(j=0; j<k; j++) {
            uint8_t jd8  = j / 8;
            uint8_t jm8  = j % 8;
            uint8_t jd32 = j / 32;
            uint8_t jm32 = j % 32;
            uint8_t parity_bit;

            /* Pick our data bit */
            uint8_t data_bit = (data[jd8] >> (7-jm8)) & 1;

            /* Pick the packed generator constant for this data bit (row)
             * and parity check (column).
             */
            uint32_t gen = g[jd32*(r/32) + id32];

            /* Rotate the constant around to the right value for our row inside
             * this block. Hopefully compiles to a ROR operation.
             */
            gen = gen >> jm32 | gen << (32-jm32);

            /* Pick our parity bit */
            parity_bit = (gen >> (31 - im32)) & 1;

            /* Add on to the parity check */
            parity += data_bit & parity_bit;
        }

        /* Store the result of the parity check */
        codeword[16 + id8] |= (parity & 1) << (7 - im8);
    }
}

void ldpc_init_paritycheck(enum ldpc_code code, uint32_t* h)
{
    int u, v, i, j, n, k, m;
    uint8_t const (* proto)[8];

    switch(code) {
        case LDPC_CODE_N128_K64:
            n = 128;
            k = 64;
            proto = h_n128_k64;
            break;

        case LDPC_CODE_N256_K128:
            n = 256;
            k = 128;
            proto = h_n256_k128;
            break;

        case LDPC_CODE_N512_K256:
            n = 512;
            k = 256;
            proto = h_n512_k256;
            break;

        default:
            return;
    }

    /* Initialise h to all-zeros so we can just OR bits on later */
    memset(h, 0x00, ((n-k)*n)/8);

    m = n/8;

    /* For each row of the prototype */
    for(u=0; u<4; u++) {
        /* For each prototype sub-matrix entry in this row */
        for(v=0; v<8; v++) {
            uint8_t subm = proto[u][v];

            /* If we're setting either an identity matrix, or a rotated
             * identity matrix, or the sum of both... */
            if((subm & HP) || (subm & HI)) {
                uint8_t rot = subm & 0x3F;

                /* For each row in the MxM sub-matrix */
                for(i=0; i<m; i++) {
                    /* For each bit in the sub-matrix row */
                    for(j=0; j<m; j++) {
                        /* Compute the uint32_t containing this bit, and the
                         * bit offset */
                        int idx =  (((u * m) + i) * (n/32)) +
                                   ((v * m)/32) +
                                   (j / 32);
                        int shift = 31 - (j % 32);

                        /* Correct for when m<32 and we pack multiple blocks
                         * into each uint32 */
                        if(m<32) {
                            shift -= m * (v % (32/m));
                        }

                        /* Work out the rotated bit and set it if matching */
                        h[idx] |= (j == (i + rot) % m) << shift;

                        /* If HI and HP were both set, do the same with a rot
                         * of 0 (to add on the identity matrix).
                         */
                        if((subm & HP) && (subm & HI)) {
                            h[idx] ^= (j == i % m) << shift;
                        }
                    }
                }
            }
        }
    }
}
