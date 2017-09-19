#include "ch.h"
#include "hal.h"
#include "m3pyro_status.h"
#include "m3pyro_hal.h"
#include "m3pyro_selftest.h"

/* Report self test failure and make the board safe. */
void m3pyro_selftest_fail(uint8_t err) {
    m3status_set_error(M3PYRO_COMPONENT_SELFTEST, err);
    m3pyro_1a_disable();
    m3pyro_3a_disable();
    m3pyro_cont_gnd();
    chThdSleepMilliseconds(10);
    m3pyro_cont_disable();
}

/* Attempt to discharge the bus and verify it did discharge. */
bool m3pyro_selftest_discharge(void) {
    m3pyro_1a_disable();
    m3pyro_3a_disable();
    m3pyro_cont_gnd();

    systime_t discharge_start = chVTGetSystemTimeX();

    while(m3pyro_read_bus() > 1) {
        chThdSleepMilliseconds(10);
        if(ST2MS(chVTTimeElapsedSinceX(discharge_start)) > 500) {
            m3pyro_cont_disable();
            m3status_set_error(M3PYRO_COMPONENT_SELFTEST,
                               M3PYRO_ERROR_SELFTEST_DISCHARGE);
            return false;
        }
    }

    m3pyro_cont_disable();
    return true;
}

/* Run a self test.
 * NOTE: This must only be run before other continuous measurement threads
 * have started, e.g. at boot, since it will turn channels on and off etc.
 */
void m3pyro_selftest(void) {
    uint8_t v;

    m3status_set_init(M3PYRO_COMPONENT_SELFTEST);

    /* Disable all bus sources and sinks. */
    m3pyro_deassert_ch();
    m3pyro_1a_disable();
    m3pyro_3a_disable();
    m3pyro_cont_disable();

    /* Discharge the bus and verify. */
    if(!m3pyro_selftest_discharge()) {
        return;
    }

    /* Energize bus to continuity measurement voltage and verify. */
    m3pyro_cont_enable();
    chThdSleepMilliseconds(300);
    v = m3pyro_read_bus();
    /* When continuity is energized the bus still has 14K impedance to ground,
     * probably through the 1A/3A supplies, so it reads 2.6V typically.
     */
    if(v < 24) {
        m3pyro_selftest_fail(M3PYRO_ERROR_SELFTEST_CONT);
        return;
    }
    m3pyro_cont_disable();
    if(!m3pyro_selftest_discharge()) {
        return;
    }

    /* Check if supply voltage is present. If not, soft-fail. */
    v = m3pyro_read_supply();
    if(v < 60) {
        m3status_set_error(M3PYRO_COMPONENT_SELFTEST,
                           M3PYRO_ERROR_SELFTEST_SUPPLY);
        return;
    }

    /* Enable 1A supply and verify bus high. */
    m3pyro_1a_enable();
    chThdSleepMilliseconds(10);
    v = m3pyro_read_bus();
    if(v < 50) {
        m3pyro_selftest_fail(M3PYRO_ERROR_SELFTEST_1A);
        return;
    }
    m3pyro_1a_disable();
    if(!m3pyro_selftest_discharge()) {
        return;
    }

    /* Enable 3A supply and verify bus high. */
    m3pyro_3a_enable();
    chThdSleepMilliseconds(10);
    v = m3pyro_read_bus();
    if(v < 50) {
        m3pyro_selftest_fail(M3PYRO_ERROR_SELFTEST_3A);
        return;
    }
    m3pyro_3a_disable();
    if(!m3pyro_selftest_discharge()) {
        return;
    }

    m3status_set_ok(M3PYRO_COMPONENT_SELFTEST);
}
