#include "ch.h"
#include "hal.h"
#include "m3can.h"
#include "m3pyro_arming.h"
#include "m3pyro_firing.h"
#include "m3pyro_status.h"

static bool m3pyro_armed = false;

static THD_WORKING_AREA(m3pyro_arming_thd_wa, 256);
static THD_FUNCTION(m3pyro_arming_thd, arg) {
    (void)arg;

    while(true) {
        m3can_send(CAN_MSG_ID_M3PYRO_ARM_STATUS, false,
                 (uint8_t*)&m3pyro_armed, 1);
        m3status_set_ok(M3PYRO_COMPONENT_ARMING);
        chThdSleepMilliseconds(500);
    }
}

void m3pyro_arming_init() {
    m3status_set_init(M3PYRO_COMPONENT_ARMING);
    chThdCreateStatic(m3pyro_arming_thd_wa, sizeof(m3pyro_arming_thd_wa),
                      NORMALPRIO, m3pyro_arming_thd, NULL);
}

void m3pyro_arming_set(bool armed) {
    if(!armed) {
        m3pyro_firing_fire(0, 0, 0, 0);
    }
    m3pyro_armed = armed;
    m3can_send(CAN_MSG_ID_M3PYRO_ARM_STATUS, false,
               (uint8_t*)&m3pyro_armed, 1);
}

bool m3pyro_arming_armed() {
    return m3pyro_armed;
}
