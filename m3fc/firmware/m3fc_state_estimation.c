/*
 * State estimation and sensor fusion
 * M3FC
 * 2014, 2016, 2017 Adam Greig, Cambridge University Spaceflight
 */

#include "ch.h"
#include <math.h>
#include "m3can.h"
#include "m3fc_config.h"
#include "m3fc_status.h"
#include "m3fc_state_estimation.h"

/* Kalman filter state and covariance storage.
 * See below for detailed description.
 */
static float x[3]    = {0.0f, 0.0f, 0.0f};
static float p[3][3] = {{250.0f, 0.0f, 0.0f},
                        {  0.0f, 0.1f, 0.0f},
                        {  0.0f, 0.0f, 0.1f}};

/* Used to compute dt for each prediction step. */
static systime_t t_clk;

/* Lock to protect the global shared Kalman state */
static binary_semaphore_t kalman_bsem;

/* Constants from the US Standard Atmosphere 1976 */
static const float Rs = 8.31432f;
static const float g0 = 9.80665f;
static const float M = 0.0289644f;
static const float Lb[7] = {
    -0.0065f, 0.0f, 0.001, 0.0028f, 0.0f, -0.0028f, -0.002f};
static const float Pb[7] = {
    101325.0f, 22632.10f, 5474.89f, 868.02f, 110.91f, 66.94f, 3.96f};
static const float Tb[7] = {
    288.15f, 216.65, 216.65, 228.65, 270.65, 270.65, 214.65};
static const float Hb[7] = {
    0.0f, 11000.0f, 20000.0f, 32000.0f, 47000.0f, 51000.0f, 71000.0f};

/* Controlled externally to indicate whether the barometer should be used
 * (not during transonic regime) and whether a high-g dynamic event is expected
 * to occur soon (ignition, separation, etc).
 */
volatile bool m3fc_state_estimation_trust_barometer;
volatile bool m3fc_state_estimation_dynamic_event_expected;

/* Functions to convert pressures to altitudes via the
 * US Standard Atmosphere
 */
static float m3fc_state_estimation_pressure_to_altitude(float pressure);
static float m3fc_state_estimation_p2a_nonzero_lapse(float p, int b);
static float m3fc_state_estimation_p2a_zero_lapse(float p, int b);

/* Internal accelerometer update function, might be used by multiple
 * accelerometers. Called from state_estimation_new_accel.
 */
static void m3fc_state_estimation_update_accel(float accel, float r);


/*
 * Run the Kalman prediction step and return the latest state estimate.
 *
 * Our Kalman state is x_k = [x_0  x_1  x_2]' = [x  dx/dt  d²x/dt²]'
 * i.e. [position  velocity  acceleration]'.
 *
 * We discretise using a velocity Verlet integrator, giving a
 * state transition matrix F:
 * F = [ 1  dt  dt²/2 ]
 *     [ 0   1  dt    ]
 *     [ 0   0   1    ]
 *
 * We model the system as undergoing a jerk d³x/dt³ whose value
 * j ~ N(0, q) which realises a constant value over each integration
 * period dt. This leads to a process noise w_k:
 *
 * delta position     = j dt³/6
 * delta velocity     = j dt²/2
 * delta acceleration = j dt
 *
 * w_k = [j.dt²/6  j.dt²/2  j.dt]'
 *
 * We then find Q as E[w_k . w_k'], where E[j²]=q, the jerk variance:
 * Q = [ q dt^6 /36    q dt^5 /12    q dt^4 /6 ]
 *     [ q dt^5 /12    q dt^4 /4     q dt^3 /2 ]
 *     [ q dt^4 /6     q dt^3 /2     q dt^2 /1 ]
 *
 * We do not model any control input (there is none) so B.u=0.
 *
 * F.P.F' is the final quantity of interest for Kalman prediction.
 *
 * F.P.F' = [ 1  dt  dt²/2 ][ P00 P01 P02 ][ 1      0   0 ]
 *          [ 0   1  dt    ][ P10 P11 P12 ][ dt     1   0 ]
 *          [ 0   0   1    ][ P20 P21 P22 ][ dt²/2  dt  1 ]
 *
 *        = [
 *           [P00 + dt P10 + dt²/2 P20,
 *            P01 + dt P11 + dt²/2 P21,
 *            P02 + dt P12 + dt²/2 P22
 *           ],
 *           [         P10 + dt    P20,
 *                     P11 + dt    P21,
 *                     P12 + dt    P22
 *           ],
 *           [                     P20,
 *                                 P21,
 *                                 P22
 *           ]
 *          ] . F'
 *
 *        = [
 *           [         P00 + dt P10 + dt²/2 P20
 *             +    dt(P01 + dt P11 + dt²/2 P21)
 *             + dt²/2(P02 + dt P12 + dt²/2 P22),
 *
 *                     P01 + dt P11 + dt²/2 P21
 *             +    dt(P02 + dt P12 + dt²/2 P22),
 *
 *                     P02 + dt P12 + dt²/2 P22
 *           ],
 *           [         P10 + dt P20
 *             +    dt(P11 + dt P21)
 *             + dt²/2(P12 + dt P22),
 *
 *                     P11 + dt P21
 *             +    dt(P12 + dt P22),
 *
 *                     P12 + dt P22
 *           ],
 *           [
 *                     P20 + dt P21 + dt²/2 P22,
 *
 *                              P21 + dt    P22,
 *
 *                                          P22
 *           ]
 *          ]
 *
 * It's just not quite awful enough to write general purpose matrix routines.
 */
state_estimate_t m3fc_state_estimation_get_state()
{
    float q, dt, dt2, dt3, dt4, dt5, dt6, dt2_2;
    state_estimate_t x_out;

    /* Set the process noise variance according to whether we expect
     * something to change soon. These numbers are more or less guesses.
     */
    if(m3fc_state_estimation_dynamic_event_expected) {
        q = 2000.0f;
    } else {
        q = 500.0f;
    }

    /* Acquire lock */
    chBSemWait(&kalman_bsem);

    /* Find elapsed time */
    dt = (float)(ST2US(chVTTimeElapsedSinceX(t_clk))) / 1e6f;
    t_clk = chVTGetSystemTimeX();

    dt2 = dt * dt;
    dt3 = dt * dt2;
    dt4 = dt * dt3;
    dt5 = dt * dt4;
    dt6 = dt * dt5;
    dt2_2 = dt2 / 2.0f;

    /* Update state
     * x_{k|k-1} = F_k x_{k-1|k-1}
     *           = [x_0 + dt x_1 + dt²/2 x_2]
     *             [         x_1 + dt    x_2]
     *             [                     x_2]
     */
    x[0] += dt * x[1] + dt2_2 * x[2];
    x[1] += dt * x[2];

    /* Update covariance
     * P_{k|k-1} = F_k P_{k-1|k-1} F'_k + Q
     * Uses F.P.F' from above. We'll add Q later, this is just the FPF'.
     * Conveniently the form means we can update each element in-place.
     */
    p[0][0] += (   dt    *  p[1][0] + dt2_2 * p[2][0]
                 + dt    * (p[0][1] + dt    * p[1][1] * dt2_2 * p[2][1])
                 + dt2_2 * (p[0][2] + dt    * p[1][2] + dt2_2 * p[2][2]));
    p[0][1] += (   dt    *  p[1][1] + dt2_2 * p[2][1]
                 + dt    * (p[0][2] + dt    * p[1][2] + dt2_2 * p[2][2]));
    p[0][2] += (   dt    *  p[1][2] + dt2_2 * p[2][2]);

    p[1][0] += (   dt    *  p[2][0]
                 + dt    * (p[1][1] + dt    * p[2][1])
                 + dt2_2 * (p[1][2] + dt    * p[2][2]));
    p[1][1] += (   dt    *  p[2][1]
                 + dt    * (p[1][2] + dt    * p[2][2]));
    p[1][2] += (   dt    *  p[2][2]);

    p[2][0] += (   dt    *  p[2][1] + dt2_2 * p[2][2]);
    p[2][1] += (   dt    *  p[2][2]);

    /* Add process noise to matrix above.
     * P_{k|k-1} += Q
     */
    p[0][0] += q * dt6 / 36.0f;
    p[0][1] += q * dt5 / 12.0f;
    p[0][2] += q * dt4 /  6.0f;
    p[1][0] += q * dt5 / 12.0f;
    p[1][1] += q * dt4 /  4.0f;
    p[1][2] += q * dt3 /  2.0f;
    p[2][0] += q * dt4 /  6.0f;
    p[2][1] += q * dt3 /  2.0f;
    p[2][2] += q * dt2 /  1.0f;

    /* Copy state to return struct */
    x_out.h = x[0];
    x_out.v = x[1];
    x_out.a = x[2];

    /* Release lock */
    chBSemSignal(&kalman_bsem);

    /* Transmit the newly predicted state and variances over CAN */
    m3can_send_f32(CAN_MSG_ID_M3FC_SE_T_H, dt, x[0], 2);
    m3can_send_f32(CAN_MSG_ID_M3FC_SE_V_A, x[1], x[2], 2);
    m3can_send_f32(CAN_MSG_ID_M3FC_SE_VAR_H, p[0][0], 0.0f, 1);
    m3can_send_f32(CAN_MSG_ID_M3FC_SE_VAR_V_A, p[1][1], p[2][2], 2);

    m3status_set_ok(M3FC_COMPONENT_SE);

    return x_out;
}

/*
 * Initialises the state estimation's shared variables.
 */
void m3fc_state_estimation_init()
{
    m3status_set_init(M3FC_COMPONENT_SE);
    m3fc_state_estimation_trust_barometer = true;
    m3fc_state_estimation_dynamic_event_expected = false;
    t_clk = chVTGetSystemTime();
    chBSemObjectInit(&kalman_bsem, false);
}

/* We run a Kalman update step with a new pressure reading.
 * The pressure is converted to an altitude (since that's what's in our state
 * and what is useful to reason about) using the US standard atmosphere via the
 * `state_estimation_pressure_to_altitude` function. We also use this function
 * to estimate the current sensor noise in altitude terms.
 *
 * We thus derive R, the sensor noise variance, as the altitude error band at
 * the current altitude, squared:
 * R = (alt(pressure-error) - alt(pressure+error))².
 *
 * Then the Kalman update is run, with:
 * z = [altitude]
 * H = [1 0 0]
 * y = z - Hx = [altitude - x_0]
 * s = H P H' + R = P00 + R
 * K = P H' s^-1 = [P00 * s^-1]
 *                 [P10 * s^-1]
 *                 [P20 * s^-1]
 * x_{k|k} = x_{k|k-1} + K . y
 *         = [x_0 + (P00 * 1/(P00+R)) * y]
 *           [x_1 + (P10 * 1/(P00+R)) * y]
 *           [x_2 + (P20 * 1/(P00+R)) * y]
 * P_{k|k} = (I - K H) P_{k|k-1}
 *         = [1-K0  0  0][P00 P01 P02]
 *           [ -K1  1  0][P10 P11 P12]
 *           [ -K2  0  1][P20 P21 P22]
 *         = [P00 - K0 P00    P01 - K0 P01    P02 - K0 P02]
 *           [P10 - K1 P00    P11 - K1 P01    P12 - K1 P02]
 *           [P20 - K2 P00    P21 - K2 P01    P22 - K2 P02]
 */
void m3fc_state_estimation_new_pressure(float pressure, float rms)
{
    float y, r, s_inv, k[3];
    float h, hp, hm;

    /* Discard data when mission control believes we are transonic. */
    if(!m3fc_state_estimation_trust_barometer)
        return;

    /* Convert pressure reading into an altitude.
     * Run the same conversion for pressure ± sensor resolution to get an idea
     * of the current noise variance in altitude terms for the filter.
     */
    h = m3fc_state_estimation_pressure_to_altitude(pressure);
    hp = m3fc_state_estimation_pressure_to_altitude(pressure + rms);
    hm = m3fc_state_estimation_pressure_to_altitude(pressure - rms);
    r = (hm - hp) * (hm - hp);

    /* If there was an error (couldn't find suitable altitude band) for this
     * pressure, just don't use it. It's probably wrong. */
    if(h == -9999.0f || hp == -9999.0f || hm == -9999.0f) {
        m3status_set_error(M3FC_COMPONENT_SE, M3FC_ERROR_SE_PRESSURE);
        return;
    }

    /* Acquire lock */
    chBSemWait(&kalman_bsem);

    /* Measurement residual */
    y = h - x[0];

    /* Precision */
    s_inv = 1.0f / (p[0][0] + r);

    /* Compute optimal Kalman gains */
    k[0] = p[0][0] * s_inv;
    k[1] = p[1][0] * s_inv;
    k[2] = p[2][0] * s_inv;

    /* New state after measurement */
    x[0] += k[0] * y;
    x[1] += k[1] * y;
    x[2] += k[2] * y;

    /* Update P matrix post-measurement */
    p[0][0] -= k[0] * p[0][0];
    p[0][1] -= k[0] * p[0][1];
    p[0][2] -= k[0] * p[0][2];
    p[1][0] -= k[1] * p[0][0];
    p[1][1] -= k[1] * p[0][1];
    p[1][2] -= k[1] * p[0][2];
    p[2][0] -= k[2] * p[0][0];
    p[2][1] -= k[2] * p[0][1];
    p[2][2] -= k[2] * p[0][2];

    /* Release lock */
    chBSemSignal(&kalman_bsem);
}

static float m3fc_state_estimation_pressure_to_altitude(float pressure)
{
    int b;
    /* If the pressure is below the model's ground level, we can
     * extrapolate into the ground instead.
     */
    if(pressure > Pb[0]) {
        return m3fc_state_estimation_p2a_nonzero_lapse(pressure, 0);
    }

    /* For each level of the US Standard Atmosphere 1976, check if the pressure
     * is inside that level, and use the appropriate conversion based on lapse
     * rate at that level.
     */
    for(b = 0; b < 6; b++) {
        if(pressure <= Pb[b] && pressure > Pb[b+1]) {
            if(Lb[b] == 0.0f) {
                return m3fc_state_estimation_p2a_zero_lapse(pressure, b);
            } else {
                return m3fc_state_estimation_p2a_nonzero_lapse(pressure, b);
            }
        }
    }

    /* If no levels matched, something is wrong, returning -9999f will cause
     * this pressure value to be ignored.
     */
    return -9999.0f;
}

/*
 * Convert a pressure and an atmospheric level b into an altitude.
 * Rearranges the standard equation for non-zero-lapse regions,
 * P = Pb (Tb / (Tb + Lb(h - hb)))^(M g0 / R* Lb)
 */
static float m3fc_state_estimation_p2a_nonzero_lapse(float pressure, int b)
{
    const float lb = Lb[b];
    const float hb = Hb[b];
    const float pb = Pb[b];
    const float tb = Tb[b];

    return hb + tb/lb * (powf(pressure/pb, (-Rs*lb)/(g0*M)) - 1.0f);
}

/* Convert a pressure and an atmospheric level b into an altitude.
 * Reverses the standard equation for zero-lapse regions,
 * P = Pb exp( -g0 M (h-hb) / R* Tb)
 */
static float m3fc_state_estimation_p2a_zero_lapse(float pressure, int b)
{
    const float hb = Hb[b];
    const float pb = Pb[b];
    const float tb = Tb[b];

    return hb + (Rs * tb)/(g0 * M) * (logf(pressure / pb));
}

/* Update the state estimate with a new accelerometer reading.
 * We check if the configured "up" axis is near the maximum, and if so increase
 * the variance to compensate.
 * We try to remove 1g from the "up" axis, but if the overall acceleration is
 * close to 1G, we'll assume we're just not upright any more and return 0
 * acceleration with a larger variance.
 */
void m3fc_state_estimation_new_accels(float accels[3], float max, float rms)
{
    float accel;

    /* Get "up" acceleration from configuration. */
    if(m3fc_config.profile.accel_axis == M3FC_CONFIG_ACCEL_AXIS_X) {
        accel = accels[0];
    } else if(m3fc_config.profile.accel_axis == M3FC_CONFIG_ACCEL_AXIS_NX) {
        accel = -accels[0];
    } else if(m3fc_config.profile.accel_axis == M3FC_CONFIG_ACCEL_AXIS_Y) {
        accel = accels[1];
    } else if(m3fc_config.profile.accel_axis == M3FC_CONFIG_ACCEL_AXIS_NY) {
        accel = -accels[1];
    } else if(m3fc_config.profile.accel_axis == M3FC_CONFIG_ACCEL_AXIS_Z) {
        accel = accels[2];
    } else if(m3fc_config.profile.accel_axis == M3FC_CONFIG_ACCEL_AXIS_NZ) {
        accel = -accels[2];
    } else {
        m3status_set_error(M3FC_COMPONENT_ACCEL, M3FC_ERROR_ACCEL_AXIS);
    }

    float overall_accel = sqrtf(accels[0] * accels[0] +
                                accels[1] * accels[1] +
                                accels[2] * accels[2]);

    if(fabsf(overall_accel - 9.80665f) < 1.0f) {
        /* Check if overall acceleration is near 1G, and treat as zero if so */
        accel = 0.0f;
        rms = 9.80665f;
    } else {
        /* Update RMS if acceleration is above maximum. */
        if(fabsf(accel) > max) {
            rms += fabsf(accel) - max;
        }
    }

    /* Subtract 1G from the "up" acceleration to remove effect of gravity */
    accel -= 9.80665f;

    m3fc_state_estimation_update_accel(accel, rms*rms);
}


/* Run the Kalman update for a single acceleration value.
 * Called internally from the new_accel functions after
 * preprocessing.
 *
 * z = [accel]
 * H = [0 0 1]
 * y = z - Hx = [accel - x_2]
 * s = H P H' + R = P22 + R
 * K = P H' s^-1 = [P02 * s^-1]
 *                 [P12 * s^-1]
 *                 [P22 * s^-1]
 * x_{k|k} = x_{k|k-1} + K . y
 *         = [x_0 + (P02 * 1/(P22+R)) * y]
 *           [x_1 + (P12 * 1/(P22+R)) * y]
 *           [x_2 + (P22 * 1/(P22+R)) * y]
 * P_{k|k} = (I - K H) P_{k|k-1}
 *         = [1  0   -K0][P00 P01 P02]
 *           [0  1   -K1][P10 P11 P12]
 *           [0  0  1-K2][P20 P21 P22]
 *         = [P00 - K0 P20    P01 - K0 P21    P02 - K0 P22]
 *           [P10 - K1 P20    P11 - K1 P21    P12 - K1 P22]
 *           [P20 - K2 P20    P21 - K2 P21    P22 - K2 P22]
 */
static void m3fc_state_estimation_update_accel(float a, float r)
{
    float y, s_inv, k[3];

    /* Acquire lock */
    chBSemWait(&kalman_bsem);

    /* Measurement residual */
    y = a - x[2];

    /* Precision */
    s_inv = 1.0f / (p[2][2] + r);

    /* Compute optimal Kalman gains */
    k[0] = p[0][2] * s_inv;
    k[1] = p[1][2] * s_inv;
    k[2] = p[2][2] * s_inv;

    /* Update state */
    x[0] += k[0] * y;
    x[1] += k[1] * y;
    x[2] += k[2] * y;

    /* Update covariance */
    p[0][0] -= k[0] * p[2][0];
    p[0][1] -= k[0] * p[2][1];
    p[0][2] -= k[0] * p[2][2];
    p[1][0] -= k[1] * p[2][0];
    p[1][1] -= k[1] * p[2][1];
    p[1][2] -= k[1] * p[2][2];
    p[2][0] -= k[2] * p[2][0];
    p[2][1] -= k[2] * p[2][1];
    p[2][2] -= k[2] * p[2][2];

    /* Release lock */
    chBSemSignal(&kalman_bsem);
}

