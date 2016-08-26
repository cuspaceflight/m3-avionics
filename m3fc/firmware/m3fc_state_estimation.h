/*
 * State estimation and sensor fusion
 * M3FC
 * 2014, 2016 Adam Greig, Cambridge University Spaceflight
 */

#ifndef M3FC_STATE_ESTIMATION_H
#define M3FC_STATE_ESTIMATION_H

#include <stdint.h>

typedef struct { float h; float v; float a; } state_estimate_t;

/* Used to signal that the barometer is not trustworthy due to
 * transonic regime. Set by the mission control thread and read by
 * the barometer thread when it goes to update the state estimate.
 */
extern volatile bool m3fc_state_estimation_trust_barometer;

/* Update with a new pressure reading (in Pascals) */
void m3fc_state_estimation_new_pressure(float pressure);

/* Update with a new accelerometer reading (in m/s/s) */
void m3fc_state_estimation_new_accel(float accel);

/* Compute and return the latest state estimate */
state_estimate_t m3fc_state_estimation_get_state(void);

/* Initialise state estimation. Must be called before
 * update or prediction steps above are called. */
void m3fc_state_estimation_init(void);

#endif
