#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include "ldpc_encoder.h"
#include "ldpc_decoder.h"

double randn()
{
    double u1 = (double)random() / (double)RAND_MAX;
    double u2 = (double)random() / (double)RAND_MAX;
    return sqrt(-2.0f * log(u1)) * cos(2 * M_PI * u2);
}

bool check_msg(uint8_t* tx, uint8_t* rx) {
    int i;
    for(i=0; i<16; i++) {
        if(tx[i] != rx[i]) {
            return false;
        }
    }
    return true;
}

void run_trial(double snr, bool* soft_soft, bool* hard_soft, bool* hard_hard)
{
    uint8_t data_tx[16], code_tx[32], code_rx[32] = {0};
    double soft_llrs[256];
    double hard_llrs[256];
    uint8_t hard_bytes[32] = {0};
    double sigma = sqrt(1.0f / snr);
    size_t i;

    /* Random message */
    for(i=0; i<sizeof(data_tx); i++) {
        data_tx[i] = rand() & 0xFF;
    }

    ldpc_encode(data_tx, code_tx);

    /* Add random noise up to SNR, then find the soft LLR,
     * make a hard decision and record the hard LLR as log(BER) in the
     * appropriate direction, and record the hard decision, for our three
     * decoders.
     */
    for(i=0; i<256; i++) {
        uint8_t bit = (code_tx[i/8] >> (7-(i%8))) & 1;
        double x, n, y, p_y_xp, p_y_xm, p_x1_y, p_x0_y, ber, logber;
        uint8_t hard_bit;

        x = bit ? 1.0 : -1.0;
        n = randn() * sigma;
        y = x + n;
        p_y_xp = exp( -(y - 1)*(y - 1) / (2*sigma*sigma));
        p_y_xm = exp( -(y + 1)*(y + 1) / (2*sigma*sigma));
        p_x1_y = p_y_xp / (p_y_xp + p_y_xm);
        p_x0_y = 1.0f - p_x1_y;
        hard_bit = y > 0.0;
        ber = erfc(1/sigma);
        logber = log(ber);

        soft_llrs[i] = log(p_x0_y / p_x1_y);
        hard_llrs[i] = hard_bit ? logber : -logber;
        hard_bytes[i/8] |= hard_bit << (7 - (i%8));
    }

    /* Soft decode */
    if(ldpc_decode(soft_llrs, code_rx) && check_msg(code_rx, code_tx))
        *soft_soft = true;
    else
        *soft_soft = false;

    /* Soft decoder with hard information */
    if(ldpc_decode(hard_llrs, code_rx) && check_msg(code_rx, code_tx))
        *hard_soft = true;
    else
        *hard_soft = false;

    /* Hard decode */
    if(ldpc_hard_decode(hard_bytes, code_rx) && check_msg(code_rx, code_tx))
        *hard_hard = true;
    else
        *hard_hard = false;
}

void run_trials(double snr,
                double* soft_soft, double* hard_soft, double* hard_hard)
{
    const int max_trials = 10000;
    int n_trials;
    int n_soft_soft_errs = 0, n_hard_soft_errs = 0, n_hard_hard_errs = 0;
    for(n_trials=0; n_trials<max_trials; n_trials++) {
        bool soft_soft_ok, hard_soft_ok, hard_hard_ok;
        run_trial(snr, &soft_soft_ok, &hard_soft_ok, &hard_hard_ok);
        n_soft_soft_errs += !soft_soft_ok;
        n_hard_soft_errs += !hard_soft_ok;
        n_hard_hard_errs += !hard_hard_ok;
    }
    *soft_soft = (double)n_soft_soft_errs / (double)n_trials;
    *hard_soft = (double)n_hard_soft_errs / (double)n_trials;
    *hard_hard = (double)n_hard_hard_errs / (double)n_trials;
}

int main(int argc, char* argv[])
{
    size_t i;
    (void)argc; (void)argv;
    /*srand(time(NULL));*/
    srand(0);

    double snr_dbs[] = {
        0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0,
        4.5, 5.0, 5.5, 6.0, 6.5, 7.0, 7.5, 8.0
    };

    double soft_softs[sizeof(snr_dbs)/sizeof(double)];
    double hard_softs[sizeof(snr_dbs)/sizeof(double)];
    double hard_hards[sizeof(snr_dbs)/sizeof(double)];

    for(i=0; i<sizeof(snr_dbs)/sizeof(double); i++) {
        printf("Running trial %lu of %lu\n", i+1, sizeof(snr_dbs)/sizeof(double)+1);
        double snr = pow(10.0f, snr_dbs[i]/10.0f);
        run_trials(snr, &soft_softs[i], &hard_softs[i], &hard_hards[i]);
    }

    printf("SNRs (dB): ");
    for(i=0; i<sizeof(snr_dbs)/sizeof(double); i++) {
        printf("%.1f, ", snr_dbs[i]);
    }
    printf("\n");

    printf("soft soft: ");
    for(i=0; i<sizeof(snr_dbs)/sizeof(double); i++) {
        printf("%.3e, ", soft_softs[i]);
    }
    printf("\n");

    printf("hard soft: ");
    for(i=0; i<sizeof(snr_dbs)/sizeof(double); i++) {
        printf("%.3e, ", hard_softs[i]);
    }
    printf("\n");

    printf("hard hard: ");
    for(i=0; i<sizeof(snr_dbs)/sizeof(double); i++) {
        printf("%.3e, ", hard_hards[i]);
    }
    printf("\n");

    return 0;
}
