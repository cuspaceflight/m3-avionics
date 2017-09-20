#include <math.h>
#include "ch.h"
#include "hal.h"
#include "m3can.h"
#include "m3pyro_status.h"
#include "m3pyro_hal.h"
#include "m3pyro_continuity.h"

BSEMAPHORE_DECL(stop_bsem, true);
BSEMAPHORE_DECL(stop_ack_bsem, true);
BSEMAPHORE_DECL(restart_bsem, true);
volatile bool estop = false;

/* This function delays for at least the given ms, but if another thread calls
 * m3pyro_continuity_stop(), it will disable continuity measurement until
 * m3pyro_continuity_restart() is called, and only then return to the caller of
 * cont_wait().
 *
 * Returns true if the wait was uneventful; false if we were interrupted.
 */
bool cont_wait(uint8_t ms) {
    msg_t result = chBSemWaitTimeout(&stop_bsem, MS2ST(ms));
    if(result == MSG_TIMEOUT) {
        /* We weren't interrupted, so just return normally. */
        return true;
    } else {
        /* We were signalled while waiting. Stop continuity. */
        m3pyro_deassert_ch();
        m3pyro_cont_disable();

        /* Acknowledge the stop, so the stop() caller can get on with it. */
        chBSemSignal(&stop_ack_bsem);

        /* Wait for restart. */
        chBSemWait(&restart_bsem);

        /* Let the caller know we were interrupted while waiting. */
        return false;
    }
}

static THD_WORKING_AREA(cont_thd_wa, 512);
static THD_FUNCTION(cont_thd, arg) {
    (void)arg;
    chRegSetThreadName("cont");

    while(true) {
        uint8_t readings[8];

        for(uint8_t ch=1; ch<=8; ch++) {
            /* Charge bus up to around 2.6V with continuity supply. */
            m3pyro_cont_enable();

            /* Wait for bus to charge. */
            /* TODO: confirm if this delay is long enough */
            if(!cont_wait(150)) {
                ch--;
                continue;
            }

            /* Measure the charged bus voltage */
            adcsample_t charged_voltage = m3pyro_read_cont();

            /* Turn off continuity supply */
            m3pyro_cont_disable();

            /* Connect this channel to continuity measurement */
            if(!estop) {
                m3pyro_1a_disable();
                m3pyro_3a_disable();
                m3pyro_assert_ch(ch);
            } else {
                return;
            }

            /* Wait for any connected load to discharge the bus somewhat. */
            /* TODO: confirm this delay */
            const uint8_t discharge_ms = 10;
            if(!cont_wait(discharge_ms)) {
                ch--;
                continue;
            }

            /* Disconnect the channel */
            if(!estop) {
                m3pyro_deassert_ch();
            } else {
                return;
            }

            /* Find the resistance corresponding to this discharge */
            adcsample_t discharged_voltage = m3pyro_read_cont();
            const float capacitance = 22e-6;
            const float time = (float)discharge_ms/1000.0f;
            const float logdiff = logf( (float)discharged_voltage
                                       /(float)charged_voltage);
            float resistance = -time / (capacitance * logdiff) - 300.0f;
            if(resistance < 0.0f) resistance = 0.0f;
            if(resistance > 255.0f) resistance = 255.0f;
            readings[ch-1] = (uint8_t)resistance;
        }

        /* Send these readings out over CAN. */
        if(!estop) {
            m3can_send(CAN_MSG_ID_M3PYRO_CONTINUITY, false, readings, 8);
            m3status_set_ok(M3PYRO_COMPONENT_CONTINUITY);
        } else {
            return;
        }
    }
}

/* Initialise the continuity measurements and begin continuous measurements. */
void m3pyro_continuity_init(void) {
    m3status_set_init(M3PYRO_COMPONENT_CONTINUITY);

    chThdCreateStatic(cont_thd_wa, sizeof(cont_thd_wa), NORMALPRIO,
                      cont_thd, NULL);
}

/* Stop continuity measurements. Disconnects continuity source from bus.
 * This method will block until continuity measurement has ceased, for at
 * most 100ms.
 */
void m3pyro_continuity_stop(void) {
    chBSemSignal(&stop_bsem);
    msg_t result = chBSemWaitTimeout(&stop_ack_bsem, MS2ST(100));

    if(result == MSG_TIMEOUT) {
        /* If we timed out waiting for ACK, we'll take matters into
         * our own hands and disable continuity and stop the cont thread.
         * NB if the continuity thread did continue looping, it would
         * cause multiple channels to fire while supplies are enabled,
         * which is obviously bad.
         * Ideally we would kill the thread in this situation, but
         * ChibiOS doesn't provide for that, so we use this estop flag.
         */
        estop = true;
        m3pyro_deassert_ch();
        m3pyro_cont_disable();
        m3status_set_error(M3PYRO_COMPONENT_CONTINUITY, M3PYRO_ERROR_ESTOP);
    }
}

/* Restart continuity measurements. */
void m3pyro_continuity_restart(void) {
    chBSemSignal(&restart_bsem);
}
