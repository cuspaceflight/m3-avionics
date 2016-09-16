#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "ldpc_codes.h"
#include "ldpc_encoder.h"
#include "ldpc_decoder.h"

#define PI (3.14159265359)

const enum ldpc_code CODE = LDPC_CODE_N1280_K1024;
uint32_t g[1024][8];
uint32_t h[384][44];
uint16_t ci[4992], vi[4992], cs[385], vs[1409];
int n=0, k=0, p=0;

/* Simple uniform to normal RNG */
static double randn()
{
    double u1 = (double)rand() / (double)RAND_MAX;
    double u2 = (double)rand() / (double)RAND_MAX;
    return sqrt(-2.0 * log(u1)) * cos(2 * PI * u2);
}

static double msg_ber(uint8_t* tx, uint8_t* rx)
{
    int i;
    int errs = 0;
    for(i=0; i<k; i++) {
        uint8_t txbit = (tx[i/8] >> (7-(i%8))) & 1;
        uint8_t rxbit = (rx[i/8] >> (7-(i%8))) & 1;
        if(txbit != rxbit) {
            errs += 1;
        }
    }
    return (double)errs / (double)k;
}

typedef struct {
    bool soft_decoded, hard_ber_decoded, hard_mp_decoded, hard_bf_decoded;
    double soft_ber, hard_ber_ber, hard_mp_ber, hard_bf_ber, uncoded_ber;
    double soft_cer, hard_ber_cer, hard_mp_cer, hard_bf_cer, uncoded_cer;
    double soft_ucer, hard_ber_ucer, hard_mp_ucer, hard_bf_ucer;
} trial_result;

void run_trial(double ebn0, trial_result* result)
{
    int i;
    double coded_eb = (double)n / (double)k;
    double coded_sigma = sqrt(coded_eb / (2*ebn0));
    double uncoded_eb = 1.0;
    double uncoded_sigma = sqrt(uncoded_eb / (2*ebn0));

    uint8_t txdata[n];
    uint8_t txcode[n+k];
    uint8_t rxdata[n+k+p];
    uint8_t workingb[n/8 + n];
    float workingf[9984];
    float rxllrs_soft[n];
    float rxllrs_hard[n];
    float rxllrs_hard_ber[n];
    uint8_t rxcode_hard[n+p];

    /* Random TX message */
    for(i=0; i<k/8; i++) {
        txdata[i] = rand() & 0xFF;
    }

    ldpc_encode_fast(CODE, (uint32_t*)g, txdata, txcode);

    /* Uncoded trial */
    memset(rxdata, 0, n);
    for(i=0; i<n; i++) {
        uint8_t txbit = (txcode[i/8] >> (7-(i%8))) & 1;
        double x = txbit ? 1.0 : -1.0;
        double n = randn() * uncoded_sigma;
        double y = x + n;
        uint8_t rxbit = y > 0;
        rxdata[i/8] |= rxbit << (7-(i%8));
    }
    result->uncoded_ber = msg_ber(txdata, rxdata);

    /* Coded trial setup */
    memset(rxcode_hard, 0, n/8);
    for(i=0; i<n; i++) {
        uint8_t bit = (txcode[i/8] >> (7-(i%8))) & 1;
        double x, n, y, p_y_x1, p_y_x0, p_x0_y, p_x1_y, ber, logber;
        uint8_t hard_bit;

        x = bit ? 1.0 : -1.0;
        n = randn() * coded_sigma;
        y = x + n;
        p_y_x1 = exp(-(y-1)*(y-1) / (2*coded_sigma*coded_sigma));
        p_y_x0 = exp(-(y+1)*(y+1) / (2*coded_sigma*coded_sigma));
        p_x1_y = p_y_x1 / (p_y_x1 + p_y_x0);
        p_x0_y = 1.0 - p_x1_y;
        hard_bit = y > 0.0;
        ber = 0.5 * erfc(1/(sqrt(2.0)*coded_sigma));
        logber = log(ber);
        rxllrs_soft[i] = log(p_x0_y / p_x1_y);
        rxllrs_hard_ber[i] = hard_bit ? logber : -logber;
        rxllrs_hard[i] = hard_bit ? -10.0 : 10.0;
        rxcode_hard[i/8] |= hard_bit << (7 - (i%8));
    }

    /* Decoding attempts */
    result->soft_decoded = ldpc_decode_mp(CODE, ci, cs, vi, vs,
                                          rxllrs_soft, rxdata, workingf);
    result->soft_ber = msg_ber(txdata, rxdata);

    result->hard_ber_decoded = ldpc_decode_mp(CODE, ci, cs, vi, vs,
                                              rxllrs_hard_ber, rxdata,
                                              workingf);
    result->hard_ber_ber = msg_ber(txdata, rxdata);

    result->hard_mp_decoded = ldpc_decode_mp(CODE, ci, cs, vi, vs,
                                             rxllrs_hard, rxdata, workingf);
    result->hard_mp_ber = msg_ber(txdata, rxdata);

    result->hard_bf_decoded = ldpc_decode_bf(CODE, ci, cs, rxcode_hard,
                                             rxdata, workingb);
    result->hard_bf_ber = msg_ber(txdata, rxdata);
}

void run_trials(double ebn0, const int max_trials, trial_result* result)
{
    int n_trials = 0;

    memset(result, 0, sizeof(trial_result));

    #pragma omp parallel for
    for(n_trials=0; n_trials<max_trials; n_trials++) {
        trial_result r;
        run_trial(ebn0, &r);
        if(r.soft_ber != 0) result->soft_cer+=1.0;
        if(r.hard_ber_ber != 0) result->hard_ber_cer+=1.0;
        if(r.hard_mp_ber != 0) result->hard_mp_cer+=1.0;
        if(r.hard_bf_ber != 0) result->hard_bf_cer+=1.0;
        if(r.uncoded_ber != 0) result->uncoded_cer+=1.0;
        if(r.soft_ber != 0 && r.soft_decoded) result->soft_ucer+=1.0;
        if(r.hard_ber_ber != 0 && r.hard_ber_decoded) result->hard_ber_ucer+=1.0;
        if(r.hard_mp_ber != 0 && r.hard_mp_decoded) result->hard_mp_ucer+=1.0;
        if(r.hard_bf_ber != 0 && r.hard_bf_decoded) result->hard_bf_ucer+=1.0;
        result->soft_ber += r.soft_ber;
        result->hard_ber_ber += r.hard_ber_ber;
        result->hard_mp_ber += r.hard_mp_ber;
        result->hard_bf_ber += r.hard_bf_ber;
        result->uncoded_ber += r.uncoded_ber;
    }

    result->soft_ber /= (double)max_trials;
    result->hard_ber_ber /= (double)max_trials;
    result->hard_mp_ber /= (double)max_trials;
    result->hard_bf_ber /= (double)max_trials;
    result->uncoded_ber /= (double)max_trials;
    result->soft_cer /= (double)max_trials;
    result->hard_ber_cer /= (double)max_trials;
    result->hard_mp_cer /= (double)max_trials;
    result->hard_bf_cer /= (double)max_trials;
    result->uncoded_cer /= (double)max_trials;
    result->soft_ucer /= (double)max_trials;
    result->hard_ber_ucer /= (double)max_trials;
    result->hard_mp_ucer /= (double)max_trials;
    result->hard_bf_ucer /= (double)max_trials;

}

#define PRINTRESULT(prop) printf(#prop " = ["); for(i=0; i<n_ebn0s; i++) printf("%.3e, ", results[i].prop); printf("]\n")

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    const int max_trials = 1000;

    size_t i;

    double ebn0_dbs[] = {
        0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0,
        4.5, 5.0, 5.5, 6.0, 6.5, 7.0, 7.5, 8.0
    };

    const size_t n_ebn0s = 16;
    trial_result results[n_ebn0s];

    srand(0);

    ldpc_codes_get_params(CODE, &n, &k, &p, NULL, NULL, NULL);

    printf("# Starting simulations, n=%d k=%d p=%d, %d repeats\n",
           n, k, p, max_trials);

    ldpc_codes_init_generator(CODE, (uint32_t*)g);
    ldpc_codes_init_paritycheck(CODE, (uint32_t*)h);
    ldpc_codes_init_sparse_paritycheck(CODE, (uint32_t*)h, ci, cs, vi, vs);

    for(i=0; i<sizeof(ebn0_dbs)/sizeof(double); i++) {
        printf("# Running trial %lu of %lu: Eb/N0=%.1fdB\n",
               i+1, sizeof(ebn0_dbs)/sizeof(double), ebn0_dbs[i]);
        double ebn0 = pow(10.0f, ebn0_dbs[i]/10.0f);
        run_trials(ebn0, max_trials, &results[i]);
    }

    printf("ebn0_db = [ ");
    for(i=0; i<sizeof(ebn0_dbs)/sizeof(double); i++) {
        printf("%.1f, ", ebn0_dbs[i]);
    }
    printf("]\n");

    PRINTRESULT(soft_ber);
    PRINTRESULT(hard_ber_ber);
    PRINTRESULT(hard_mp_ber);
    PRINTRESULT(hard_bf_ber);
    PRINTRESULT(uncoded_ber);
    PRINTRESULT(soft_cer);
    PRINTRESULT(hard_ber_cer);
    PRINTRESULT(hard_mp_cer);
    PRINTRESULT(hard_bf_cer);
    PRINTRESULT(uncoded_cer);
    PRINTRESULT(soft_ucer);
    PRINTRESULT(hard_ber_ucer);
    PRINTRESULT(hard_mp_ucer);
    PRINTRESULT(hard_bf_ucer);

    return 0;
}
