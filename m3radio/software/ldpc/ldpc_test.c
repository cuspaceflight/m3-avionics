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

void run_trial(double ebn0, bool* soft_soft,
               bool* hard_soft, bool* hard_hard, bool* uncoded)
{
    uint8_t data_tx[16], code_tx[32], code_rx[32] = {0};
    double soft_llrs[256];
    double hard_llrs[256];
    uint8_t hard_bytes[32] = {0};
    const double coded_eb = 2.0;
    double coded_sigma = sqrt(coded_eb / (2*ebn0));
    double uncoded_eb = 1.0;
    double uncoded_sigma = sqrt(uncoded_eb / (2*ebn0));
    size_t i;

    /* Random message */
    for(i=0; i<sizeof(data_tx); i++) {
        data_tx[i] = rand() & 0xFF;
    }

    ldpc_encode(data_tx, code_tx);

    /* Run a simple uncoded trial with the 128 user bits and the appropriate
     * sigma for the uncoded case (since Eb is half, for the same EbN0 we need
     * a different sigma)
     */
    *uncoded = true;
    for(i=0; i<128; i++) {
        uint8_t bit = (code_tx[i/8] >> (7-(i%8))) & 1;
        double x = bit ? 1.0 : -1.0;
        double n = randn() * uncoded_sigma;
        double y = x + n;
        if((y > 0.0) != bit) {
            *uncoded = false;
        }
    }

    /* Add random noise up to Eb/N0, then find the soft LLR,
     * make a hard decision and record the hard LLR as log(BER) in the
     * appropriate direction, and record the hard decision, for our three
     * LDPC decoders.
     */
    for(i=0; i<256; i++) {
        uint8_t bit = (code_tx[i/8] >> (7-(i%8))) & 1;
        double x, n, y, p_y_xp, p_y_xm, p_x1_y, p_x0_y, ber, logber;
        uint8_t hard_bit;

        x = bit ? 1.0 : -1.0;
        n = randn() * coded_sigma;
        y = x + n;
        p_y_xp = exp( -(y - 1)*(y - 1) / (2*coded_sigma*coded_sigma));
        p_y_xm = exp( -(y + 1)*(y + 1) / (2*coded_sigma*coded_sigma));
        p_x1_y = p_y_xp / (p_y_xp + p_y_xm);
        p_x0_y = 1.0f - p_x1_y;
        hard_bit = y > 0.0;
        ber = erfc(1/coded_sigma);
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

void run_trials(double ebn0, double* soft_soft, double* hard_soft,
                double* hard_hard, double* uncoded)
{
    const int max_trials = 10000;
    int n_trials = 0;
    int n_soft_soft_errs = 0, n_hard_soft_errs = 0, n_hard_hard_errs = 0;
    int n_uncoded_errs = 0;
    #pragma omp parallel for
    for(n_trials=0; n_trials<max_trials; n_trials++) {
        bool soft_soft_ok, hard_soft_ok, hard_hard_ok, uncoded_ok;
        run_trial(ebn0, &soft_soft_ok, &hard_soft_ok, &hard_hard_ok, &uncoded_ok);
        n_soft_soft_errs += !soft_soft_ok;
        n_hard_soft_errs += !hard_soft_ok;
        n_hard_hard_errs += !hard_hard_ok;
        n_uncoded_errs += !uncoded_ok;
    }
    *soft_soft = (double)n_soft_soft_errs / (double)max_trials;
    *hard_soft = (double)n_hard_soft_errs / (double)max_trials;
    *hard_hard = (double)n_hard_hard_errs / (double)max_trials;
    *uncoded   = (double)n_uncoded_errs   / (double)max_trials;
}

int main(int argc, char* argv[])
{
    size_t i;
    (void)argc; (void)argv;
    srandom(0);

    double ebn0_dbs[] = {
        0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0,
        4.5, 5.0, 5.5, 6.0, 6.5, 7.0, 7.5, 8.0
    };

    double soft_softs[sizeof(ebn0_dbs)/sizeof(double)];
    double hard_softs[sizeof(ebn0_dbs)/sizeof(double)];
    double hard_hards[sizeof(ebn0_dbs)/sizeof(double)];
    double uncoded[   sizeof(ebn0_dbs)/sizeof(double)];

    for(i=0; i<sizeof(ebn0_dbs)/sizeof(double); i++) {
        printf("Running trial %lu of %lu\n", i+1, sizeof(ebn0_dbs)/sizeof(double)+1);
        double ebn0 = pow(10.0f, ebn0_dbs[i]/10.0f);
        run_trials(ebn0, &soft_softs[i], &hard_softs[i], &hard_hards[i], &uncoded[i]);
    }

    printf("Eb/N0 (dB): ");
    for(i=0; i<sizeof(ebn0_dbs)/sizeof(double); i++) {
        printf("%.1f, ", ebn0_dbs[i]);
    }
    printf("\n");

    printf("soft soft: ");
    for(i=0; i<sizeof(ebn0_dbs)/sizeof(double); i++) {
        printf("%.3e, ", soft_softs[i]);
    }
    printf("\n");

    printf("hard soft: ");
    for(i=0; i<sizeof(ebn0_dbs)/sizeof(double); i++) {
        printf("%.3e, ", hard_softs[i]);
    }
    printf("\n");

    printf("hard hard: ");
    for(i=0; i<sizeof(ebn0_dbs)/sizeof(double); i++) {
        printf("%.3e, ", hard_hards[i]);
    }
    printf("\n");

    printf("uncoded:   ");
    for(i=0; i<sizeof(ebn0_dbs)/sizeof(double); i++) {
        printf("%.3e, ", uncoded[i]);
    }
    printf("\n");

    return 0;
}
