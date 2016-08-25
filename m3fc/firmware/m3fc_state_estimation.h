/*
 * State estimation and sensor fusion
 * M2FC
 * 2014 Adam Greig, Cambridge University Spaceflight
 */

#ifndef STATE_ESTIMATION_H
#define STATE_ESTIMATION_H

#include <stdint.h>

typedef struct { float h; float v; float a; } state_estimate_t;

/* Used to signal that the barometer is not trustworthy due to
 * transonic regime. Set by the mission control thread and read by
 * the barometer thread when it goes to update the state estimate.
 */
extern volatile uint8_t state_estimation_trust_barometer;

/* Update with a new pressure reading (in Pascals) */
void state_estimation_new_pressure(float pressure);

/* Update with a new low-g accelerometer reading (in m/s) */
void state_estimation_new_accel(float lg_accel);

/* Compute and return the latest state estimate */
state_estimate_t state_estimation_get_state(void);

/* Initialise state estimation. Must be called before
 * update or prediction steps above are called. */
void state_estimation_init(void);

#endif /* STATE_ESTIMATION_H */
