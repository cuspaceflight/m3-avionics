#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>

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

void run_trial(double ebn0, double* kbps)
{
    int i;
    double coded_eb = (double)n / (double)k;
    double coded_sigma = sqrt(coded_eb / (2*ebn0));

    uint8_t txdata[n];
    uint8_t txcode[n+k];
    uint8_t rxdata[n+k+p];
    float workingf[9984];
    float rxllrs_soft[n];

    struct timespec t_start, t_end;
    double time_taken;

    /* Random TX message */
    for(i=0; i<k/8; i++) {
        txdata[i] = rand() & 0xFF;
    }

    ldpc_encode_fast(CODE, (uint32_t*)g, txdata, txcode);

    /* Coded trial setup */
    for(i=0; i<n; i++) {
        uint8_t bit = (txcode[i/8] >> (7-(i%8))) & 1;
        double x, n, y, p_y_x1, p_y_x0, p_x0_y, p_x1_y;

        x = bit ? 1.0 : -1.0;
        n = randn() * coded_sigma;
        y = x + n;
        p_y_x1 = exp(-(y-1)*(y-1) / (2*coded_sigma*coded_sigma));
        p_y_x0 = exp(-(y+1)*(y+1) / (2*coded_sigma*coded_sigma));
        p_x1_y = p_y_x1 / (p_y_x1 + p_y_x0);
        p_x0_y = 1.0 - p_x1_y;
        rxllrs_soft[i] = log(p_x0_y / p_x1_y);
    }

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t_start);
    ldpc_decode_mp(CODE, ci, cs, vi, vs, rxllrs_soft, rxdata, workingf);
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t_end);

    time_taken = (double)t_end.tv_sec + (double)t_end.tv_nsec * 1e-9 -
                 (double)t_start.tv_sec - (double)t_start.tv_nsec * 1e-9;

    *kbps = ((double)k/1024.0) / time_taken;
}

void run_trials(double ebn0, const int max_trials, double* avg_kbps)
{
    int n_trials = 0;

    #pragma omp parallel for
    for(n_trials=0; n_trials<max_trials; n_trials++) {
        double kbps;
        run_trial(ebn0, &kbps);
        *avg_kbps += kbps;
    }

    *avg_kbps /= (double)max_trials;

}

#define PRINTRESULT(prop) printf(#prop " = ["); for(i=0; i<n_ebn0s; i++) printf("%.3e, ", results[i].prop); printf("]\n")

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    const int max_trials = 1000;

    size_t i;

    double ebn0_dbs[] = {
        0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0,
        4.5, 5.0, 5.5, 6.0, 6.5, 7.0, 7.5, 8.0,
        8.5, 9.0, 9.5, 10.0, 10.5, 11.0, 11.5, 12.0,
    };

    const size_t n_ebn0s = 24;
    double kbpss[n_ebn0s];

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
        run_trials(ebn0, max_trials, &kbpss[i]);
    }

    printf("ebn0_db = [ ");
    for(i=0; i<sizeof(ebn0_dbs)/sizeof(double); i++) {
        printf("%.1f, ", ebn0_dbs[i]);
    }
    printf("]\n");

    printf("kbps_%d_%d = [", n, k);
    for(i=0; i<n_ebn0s; i++)
        printf("%.3e, ", kbpss[i]);
    printf("]\n");

    return 0;
}
