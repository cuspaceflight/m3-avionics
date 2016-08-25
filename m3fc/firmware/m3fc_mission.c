#include <math.h>
#include "ch.h"
#include "m3fc_mission.h"
#include "m3fc_state_estimation.h"

typedef enum {
    STATE_PAD = 0, STATE_IGNITION, STATE_POWERED_ASCENT, STATE_FREE_ASCENT,
    STATE_APOGEE, STATE_DROGUE_DESCENT, STATE_RELEASE_MAIN,
    STATE_MAIN_DESCENT, STATE_LAND, STATE_LANDED, NUM_STATES
} state_t;

struct instance_data {
    int32_t t_launch;
    int32_t t_apogee;
    float h_ground;
    state_estimate_t state;
};

typedef struct instance_data instance_data_t;

typedef state_t state_func_t(instance_data_t *data);

state_t run_state(state_t cur_state, instance_data_t *data);
static state_t do_state_pad(instance_data_t *data);
static state_t do_state_ignition(instance_data_t *data);
static state_t do_state_powered_ascent(instance_data_t *data);
static state_t do_state_free_ascent(instance_data_t *data);
static state_t do_state_apogee(instance_data_t *data);
static state_t do_state_drogue_descent(instance_data_t *data);
static state_t do_state_release_main(instance_data_t *data);
static state_t do_state_main_descent(instance_data_t *data);
static state_t do_state_land(instance_data_t *data);
static state_t do_state_landed(instance_data_t *data);

state_func_t* const state_table[NUM_STATES] = {
    do_state_pad, do_state_ignition, do_state_powered_ascent,
    do_state_free_ascent, do_state_apogee, do_state_drogue_descent,
    do_state_release_main, do_state_main_descent, do_state_land,
    do_state_landed
};

state_t run_state(state_t cur_state, instance_data_t *data) {
    return state_table[cur_state](data);
};

static state_t do_state_pad(instance_data_t *data)
{
    state_estimation_trust_barometer = true;
    data->h_ground = data->state.h;
    if(chTimeNow() < 30000)
        return STATE_PAD;
    else if(data->state.a > conf.ignition_accel)
        return STATE_IGNITION;
    else
        return STATE_PAD;
}

static state_t do_state_ignition(instance_data_t *data)
{
    state_estimation_trust_barometer = false;
    data->t_launch = chTimeNow();
    return STATE_POWERED_ASCENT;
}

static state_t do_state_powered_ascent(instance_data_t *data)
{
    state_estimation_trust_barometer = false;
    if(data->state.a < 0.0f)
        return STATE_FREE_ASCENT;
    else if(chTimeElapsedSince(data->t_launch) > conf.burnout_time)
        return STATE_FREE_ASCENT;
    else
        return STATE_POWERED_ASCENT;
}

static state_t do_state_free_ascent(instance_data_t *data)
{
    state_estimation_trust_barometer = true;
    if(data->state.v < 0.0f)
        return STATE_APOGEE;
    else if(chTimeElapsedSince(data->t_launch) > conf.apogee_time)
        return STATE_APOGEE;
    else
        return STATE_FREE_ASCENT;
}

static state_t do_state_apogee(instance_data_t *data)
{
    state_estimation_trust_barometer = true;
    data->t_apogee = chTimeNow();
    pyro_fire_drogue();
    return STATE_DROGUE_DESCENT;
}

static state_t do_state_drogue_descent(instance_data_t *data)
{
    state_estimation_trust_barometer = true;
    if((data->state.h - data->h_ground) < conf.main_altitude)
        return STATE_RELEASE_MAIN;
    else if(chTimeElapsedSince(data->t_apogee) > conf.main_time)
        return STATE_RELEASE_MAIN;
    else
        return STATE_DROGUE_DESCENT;
}

static state_t do_state_release_main(instance_data_t *data)
{
    (void)data;
    state_estimation_trust_barometer = true;
    pyro_fire_main();
    return STATE_MAIN_DESCENT;
}

static state_t do_state_main_descent(instance_data_t *data)
{
    state_estimation_trust_barometer = true;
    if(chTimeElapsedSince(data->t_apogee) > conf.landing_time)
        return STATE_LAND;
    else if(fabsf(data->state.v) < 0.5f)
        return STATE_LAND;
    else
        return STATE_MAIN_DESCENT;
}

static state_t do_state_land(instance_data_t *data)
{
    (void)data;
    state_estimation_trust_barometer = true;
    return STATE_LANDED;
}

static state_t do_state_landed(instance_data_t *data)
{
    state_estimation_trust_barometer = true;
    (void)data;
    return STATE_LANDED;
}

static THD_WORKING_AREA(mission_thread_wa, 512);
static THD_FUNCTION(mission_thread, arg) {
    (void)arg;
    state_t cur_state = STATE_PAD;
    state_t new_state;
    instance_data_t data;
    data.t_launch = 0;
    data.t_apogee = 0;
    data.h_ground = 0.0f;

    while(1) {
        /* Run Kalman prediction step */
        data.state = state_estimation_get_state();

        /* Run state machine current state function */
        new_state = run_state(cur_state, &data);

        /* Log changes in state */
        if(new_state != cur_state) {
            cur_state = new_state;
        }

        /* Tick the state machine about every 10ms */
        chThdSleepMilliseconds(10);
    }
}

void m3fc_mission_init() {
    chThdCreateStatic(mission_thread_wa, sizeof(mission_thread_wa),
                      NORMALPRIO, NULL);
}
