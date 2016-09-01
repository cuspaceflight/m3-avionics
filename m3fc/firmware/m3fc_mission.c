#include <math.h>
#include "ch.h"
#include "m3fc_mission.h"
#include "m3fc_status.h"
#include "m3can.h"
#include "m3fc_config.h"
#include "m3fc_state_estimation.h"

#define PYRO_SUPPLY_THRESHOLD       (40)
#define PYRO_CONT_THRESHOLD         (100)

volatile bool m3fc_mission_pyro_armed = false;
volatile bool m3fc_mission_pyro_supply_good = false;
volatile bool m3fc_mission_pyro_cont_ok = false;

static volatile bool m3fc_mission_armed = false;

typedef enum {
    STATE_INIT = 0, STATE_PAD, STATE_IGNITION, STATE_POWERED_ASCENT,
    STATE_BURNOUT, STATE_FREE_ASCENT, STATE_APOGEE, STATE_DROGUE_DESCENT,
    STATE_RELEASE_MAIN, STATE_MAIN_DESCENT, STATE_LAND, STATE_LANDED,
    NUM_STATES
} state_t;

struct instance_data {
    systime_t t_launch;
    systime_t t_apogee;
    float h_ground;
    state_estimate_t state;
};

typedef struct instance_data instance_data_t;

typedef state_t state_func_t(instance_data_t *data);

static void m3fc_mission_send_state(state_t state, instance_data_t *data);
static void m3fc_mission_fire_pyro(int pyro_usage);
static void m3fc_mission_fire_drogue_pyro(void);
static void m3fc_mission_fire_main_pyro(void);
static void m3fc_mission_fire_dart_pyro(void);
static void m3fc_mission_check_pyros(void);

state_t run_state(state_t cur_state, instance_data_t *data);
static state_t do_state_init(instance_data_t *data);
static state_t do_state_pad(instance_data_t *data);
static state_t do_state_ignition(instance_data_t *data);
static state_t do_state_powered_ascent(instance_data_t *data);
static state_t do_state_burnout(instance_data_t *data);
static state_t do_state_free_ascent(instance_data_t *data);
static state_t do_state_apogee(instance_data_t *data);
static state_t do_state_drogue_descent(instance_data_t *data);
static state_t do_state_release_main(instance_data_t *data);
static state_t do_state_main_descent(instance_data_t *data);
static state_t do_state_land(instance_data_t *data);
static state_t do_state_landed(instance_data_t *data);

state_func_t* const state_table[NUM_STATES] = {
    do_state_init, do_state_pad, do_state_ignition, do_state_powered_ascent,
    do_state_burnout, do_state_free_ascent, do_state_apogee,
    do_state_drogue_descent, do_state_release_main, do_state_main_descent,
    do_state_land, do_state_landed
};

state_t run_state(state_t cur_state, instance_data_t *data) {
    return state_table[cur_state](data);
};

static state_t do_state_init(instance_data_t *data) {
    m3fc_state_estimation_trust_barometer = true;
    data->h_ground = data->state.h;

    m3fc_mission_check_pyros();

    /* We only proceed to the pad state after receiving an ARM command. */
    if(!m3fc_mission_armed) {
        return STATE_INIT;
    } else {
        m3status_set_ok(M3FC_COMPONENT_MC);
        return STATE_PAD;
    }
}

static state_t do_state_pad(instance_data_t *data)
{
    m3fc_state_estimation_trust_barometer = true;

    m3fc_mission_check_pyros();

    /* Detect ignition when the acceleration exceeds the threshold and we're
     * 10m above our ground altitude, which should be small enough to not
     * change the required acceleration threshold too much.
     */
    if(data->state.a > m3fc_config.profile.ignition_accel &&
       data->state.h > data->h_ground + 10.0f)
    {
        return STATE_IGNITION;
    } else {
        return STATE_PAD;
    }
}

static state_t do_state_ignition(instance_data_t *data)
{
    m3fc_state_estimation_trust_barometer = false;
    data->t_launch = chVTGetSystemTimeX();

    /* After ignition we proceed immediately to powered ascent
     * (the purpose of this state is to disable barometer and log the launch
     * time)
     */
    return STATE_POWERED_ASCENT;
}

static state_t do_state_powered_ascent(instance_data_t *data)
{
    m3fc_state_estimation_trust_barometer = false;

    /* We detect burnout as either negative acceleration (we've started to slow
     * down due to drag) or configured timeout since launch.
     */
    if(data->state.a < 0.0f) {
        return STATE_BURNOUT;
    } else if(ST2MS(chVTTimeElapsedSinceX(data->t_launch))
              > m3fc_config.profile.burnout_timeout * 100)
    {
        return STATE_BURNOUT;
    } else {
        return STATE_POWERED_ASCENT;
    }
}

static state_t do_state_burnout(instance_data_t *data)
{
    (void)data;
    m3fc_state_estimation_trust_barometer = true;

    if(data->state.h > (data->h_ground + 20.0f) &&
       ST2MS(chVTTimeElapsedSinceX(data->t_launch)) > 200)
    {
        /* If we're at least 20m above launch altitude, and it's been at least
         * 200ms since we detected launch, consider it a successful burn.
         */
        m3fc_mission_fire_dart_pyro();
        return STATE_FREE_ASCENT;
    } else {
        /* But if not, it was probably a false detection, so return to pad and
         * hope to God we got this right.
         */
        return STATE_PAD;
    }
}

static state_t do_state_free_ascent(instance_data_t *data)
{
    m3fc_state_estimation_trust_barometer = true;

    /* We detect apogee as negative velocity (we've started to fall) or the
     * configured timeout since launch.
     * We hope that we're still mostly upright for accelerometer purposes...
     */
    if(data->state.v < 0.0f) {
        return STATE_APOGEE;
    } else if(ST2MS(chVTTimeElapsedSinceX(data->t_launch))
              > m3fc_config.profile.apogee_timeout * 1000)
    {
        return STATE_APOGEE;
    } else {
        return STATE_FREE_ASCENT;
    }
}

static state_t do_state_apogee(instance_data_t *data)
{
    m3fc_state_estimation_trust_barometer = true;
    data->t_apogee = chVTGetSystemTimeX();
    m3fc_mission_fire_drogue_pyro();

    /* After apogee we fire the drogue and immediately enter drogue descent. */
    return STATE_DROGUE_DESCENT;
}

static state_t do_state_drogue_descent(instance_data_t *data)
{
    m3fc_state_estimation_trust_barometer = true;

    /* We detect time to release the main based either on the configured
     * altitude above ground or on the configured timeout since apogee.
     */
    if((data->state.h - data->h_ground) < m3fc_config.profile.main_altitude) {
        return STATE_RELEASE_MAIN;
    } else if(ST2MS(chVTTimeElapsedSinceX(data->t_apogee))
              > m3fc_config.profile.main_timeout * 1000)
    {
        return STATE_RELEASE_MAIN;
    } else {
        return STATE_DROGUE_DESCENT;
    }
}

static state_t do_state_release_main(instance_data_t *data)
{
    (void)data;
    m3fc_state_estimation_trust_barometer = true;
    m3fc_mission_fire_main_pyro();

    /* At main release we fire the main and move directly into main descent. */
    return STATE_MAIN_DESCENT;
}

static state_t do_state_main_descent(instance_data_t *data)
{
    m3fc_state_estimation_trust_barometer = true;

    /* Landing is detected based on the configured timeout (probably) or on the
     * velocity being suitably small.
     */
    if(ST2MS(chVTTimeElapsedSinceX(data->t_launch))
       > m3fc_config.profile.land_timeout * 10000)
    {
        return STATE_LAND;
    } else if(fabsf(data->state.v) < 0.5f) {
        return STATE_LAND;
    } else {
        return STATE_MAIN_DESCENT;
    }
}

static state_t do_state_land(instance_data_t *data)
{
    (void)data;
    m3fc_state_estimation_trust_barometer = true;

    /* In the future we might want to trigger events on landing, like disabling
     * cameras and entering power saving modes, but for now we just proceed
     * directly to landed.
     */
    return STATE_LANDED;
}

static state_t do_state_landed(instance_data_t *data)
{
    m3fc_state_estimation_trust_barometer = true;
    (void)data;

    /* Not much to do now. */
    return STATE_LANDED;
}

static void m3fc_mission_send_state(state_t state, instance_data_t *data) {
    uint8_t can_state = (uint8_t)state;
    uint32_t met;

    if(data->t_launch == 0) {
        met = 0;
    } else {
        met = ST2MS(chVTTimeElapsedSinceX(data->t_launch));
    }

    uint8_t buf[5] = {met, met>>8, met>>16, met>>24, can_state};

    can_send(CAN_MSG_ID_M3FC_MISSION_STATE, false, buf, 5);
}

static void m3fc_mission_fire_pyro(int usage) {
    uint8_t channels[4] = {0};
    if(m3fc_config.pyros.pyro_1_usage == usage) {
        channels[0] = m3fc_config.pyros.pyro_1_type;
    }
    if(m3fc_config.pyros.pyro_2_usage == usage) {
        channels[1] = m3fc_config.pyros.pyro_2_type;
    }
    if(m3fc_config.pyros.pyro_3_usage == usage) {
        channels[2] = m3fc_config.pyros.pyro_3_type;
    }
    if(m3fc_config.pyros.pyro_4_usage == usage) {
        channels[3] = m3fc_config.pyros.pyro_4_type;
    }
    can_send(CAN_MSG_ID_M3PYRO_FIRE_COMMAND, false, channels, 4);
}

static void m3fc_mission_fire_drogue_pyro() {
    m3fc_mission_fire_pyro(M3FC_CONFIG_PYRO_USAGE_DROGUE);
}

static void m3fc_mission_fire_main_pyro() {
    m3fc_mission_fire_pyro(M3FC_CONFIG_PYRO_USAGE_MAIN);
}

static void m3fc_mission_fire_dart_pyro() {
    m3fc_mission_fire_pyro(M3FC_CONFIG_PYRO_USAGE_DART);
}

static THD_WORKING_AREA(mission_thread_wa, 512);
static THD_FUNCTION(mission_thread, arg) {
    (void)arg;
    int can_counter = 0;
    state_t cur_state = STATE_INIT;
    state_t new_state;
    instance_data_t data;
    data.t_launch = 0;
    data.t_apogee = 0;
    data.h_ground = 0.0f;

    while(true) {
        /* Run Kalman prediction step */
        data.state = m3fc_state_estimation_get_state();

        /* Run state machine current state function */
        new_state = run_state(cur_state, &data);

        if(new_state != cur_state) {
            /* Log changes in state specifically */
            m3fc_mission_send_state(new_state, &data);

            /* Swap to the new state */
            cur_state = new_state;
        }

        /* Send the state every second as well */
        if(can_counter++ >= 100) {
            m3fc_mission_send_state(new_state, &data);
            can_counter = 0;
        }

        /* Tick the state machine about every 10ms */
        chThdSleepMilliseconds(10);
    }
}

void m3fc_mission_init() {
    m3status_set_init(M3FC_COMPONENT_MC);
    m3status_set_init(M3FC_COMPONENT_MC_PYRO);
    chThdCreateStatic(mission_thread_wa, sizeof(mission_thread_wa),
                      NORMALPRIO+5, mission_thread, NULL);
}

static void m3fc_mission_check_pyros(void) {
    if(!m3fc_mission_pyro_supply_good) {
        m3status_set_error(M3FC_COMPONENT_MC_PYRO, M3FC_ERROR_MC_PYRO_SUPPLY);
    } else if(!m3fc_mission_pyro_armed) {
        m3status_set_error(M3FC_COMPONENT_MC_PYRO, M3FC_ERROR_MC_PYRO_ARM);
    } else if(!m3fc_mission_pyro_cont_ok) {
        m3status_set_error(M3FC_COMPONENT_MC_PYRO, M3FC_ERROR_MC_PYRO_CONT);
    } else {
        m3status_set_ok(M3FC_COMPONENT_MC_PYRO);
    }
}

void m3fc_mission_handle_pyro_supply(uint8_t* data, uint8_t datalen){
    if(datalen != 1) {
        m3status_set_error(M3FC_COMPONENT_MC_PYRO, M3FC_ERROR_CAN_BAD_COMMAND);
        return;
    }

    if(data[0] > PYRO_SUPPLY_THRESHOLD) {
        m3fc_mission_pyro_supply_good = true;
    } else {
        m3fc_mission_pyro_supply_good = false;
    }
}

void m3fc_mission_handle_pyro_arm(uint8_t* data, uint8_t datalen){
    if(datalen != 1) {
        m3status_set_error(M3FC_COMPONENT_MC_PYRO, M3FC_ERROR_CAN_BAD_COMMAND);
        return;
    }

    if(data[0]) {
        m3fc_mission_pyro_armed = true;
    } else {
        m3fc_mission_pyro_armed = false;
    }
}

void m3fc_mission_handle_pyro_continuity(uint8_t* data, uint8_t datalen){
    if(datalen != 4) {
        m3status_set_error(M3FC_COMPONENT_MC_PYRO, M3FC_ERROR_CAN_BAD_COMMAND);
        return;
    }

    if(
        (m3fc_config.pyros.pyro_1_usage && data[0] > PYRO_CONT_THRESHOLD) ||
        (m3fc_config.pyros.pyro_2_usage && data[1] > PYRO_CONT_THRESHOLD) ||
        (m3fc_config.pyros.pyro_3_usage && data[2] > PYRO_CONT_THRESHOLD) ||
        (m3fc_config.pyros.pyro_4_usage && data[3] > PYRO_CONT_THRESHOLD))
    {
        m3fc_mission_pyro_cont_ok = false;
    } else {
        m3fc_mission_pyro_cont_ok = true;
    }
}

void m3fc_mission_handle_arm(uint8_t* data, uint8_t datalen) {
    (void)data;
    (void)datalen;

    m3fc_mission_armed = true;
}
