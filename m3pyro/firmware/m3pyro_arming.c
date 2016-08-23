#include "ch.h"
#include "hal.h"
#include "m3can.h"
#include "m3pyro_arming.h"
#include "m3pyro_firing.h"

static bool m3pyro_armed = false;

static THD_WORKING_AREA(m3pyro_arming_thd_wa, 256);
static THD_FUNCTION(m3pyro_arming_thd, arg) {
    (void)arg;

    while(true) {
        uint8_t armed = (uint8_t)m3pyro_armed;
        can_send(CAN_MSG_ID_M3PYRO_ARM_STATUS, false, &armed, 1);
        chThdSleepMilliseconds(500);
    }
}

void m3pyro_arming_init() {
    chThdCreateStatic(m3pyro_arming_thd_wa, sizeof(m3pyro_arming_thd_wa),
                      NORMALPRIO, m3pyro_arming_thd, NULL);
}

void m3pyro_arming_set(bool armed) {
    m3pyro_armed = armed;
    if(!armed) {
        m3pyro_firing_fire(0, 0, 0, 0);
    }
}

bool m3pyro_arming_armed() {
    return m3pyro_armed;
}
