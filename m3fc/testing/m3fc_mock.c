/*
 * This C file contains the actual implementations of all our mock functions.
 * Most of these functions are declared in their normal header files, except
 * for the ChibiOS ones where we have a stub header file for simplicity.
 * However none of the real C code is compiled, only this file and
 * m3fc_mission.c itself.
 */

#include "ch.h"
#include "m3fc_status.h"
#include "m3fc_config.h"
#include "m3fc_state_estimation.h"
#include "m3can.h"

/* These two globals are used by m3fc_mission */
volatile bool m3fc_state_estimation_trust_barometer = false;
struct m3fc_config m3fc_config;

/* Returns the current system time, in units of systick (=1ms for our mock) */
systime_t chVTGetSystemTimeX()
{
    return 0;
}

/* Returns the difference between two times, in units of systick */
systime_t chVTTimeElapsedSinceX(systime_t t)
{
    (void)t;
    return 0;
}

/* Sleeps the calling thread for t milliseconds...
 * Or advances our concept of the real world time by t systicks.
 */
void chThdSleepMilliseconds(uint32_t t)
{
    (void)t;
}

/* "Creates" a thread. We could just run the thd function immediately. */
void chThdCreateStatic(void* m, size_t ms, int pri, void (*thd)(void*), void* arg)
{
    (void)m;
    (void)ms;
    (void)pri;

    /* Run thd with given argument */
    thd(arg);
}

/* Store a callback to the Python function for can_send */
void (*py_can_send)(uint16_t msg_id, bool can_rtr, uint8_t *data, uint8_t datalen) = NULL;

/* Sends a packet over CAN.
 * msg_id is the packet ID, 11 bits.
 * can_rtr is the "remote transmission request" flag
 * *data points to datalen bytes of data
 */
void can_send(uint16_t msg_id, bool can_rtr, uint8_t *data, uint8_t datalen)
{
    (void)msg_id;
    (void)can_rtr;
    (void)data;
    (void)datalen;

    /* Forward this call to Python */
    if(py_can_send != NULL) {
        py_can_send(msg_id, can_rtr, data, datalen);
    }
}

/* m3fc_mission calls get_state to find out the latest estimate of the
 * rocket's altitude, velocity, and acceleration.
 */
state_estimate_t m3fc_state_estimation_get_state()
{
    state_estimate_t x_out;
    return x_out;
}

/* set_ok updates the global status of the given component.
 * we'd like to just log what the state of the component is.
 */
void m3status_set_ok(uint8_t component)
{
    (void)component;
}

/* see set_ok */
void m3status_set_init(uint8_t component)
{
    (void)component;
}

/* see set_ok */
void m3status_set_error(uint8_t component, uint8_t errorcode)
{
    (void)component;
    (void)errorcode;
}
