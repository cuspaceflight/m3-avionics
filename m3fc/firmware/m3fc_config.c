#include "m3fc_config.h"
#include "m3can.h"
#include "m3fc_status.h"

struct m3fc_config m3fc_config;

static THD_WORKING_AREA(m3fc_config_reporter_thd_wa, 256);
static THD_FUNCTION(m3fc_config_reporter_thd, arg) {
    (void)arg;

    while(true) {
        /* Send current config over CAN */
        can_send(CAN_MSG_ID_M3FC_CFG_PROFILE, false,
                 (uint8_t*)&m3fc_config.profile, 8);
        can_send(CAN_MSG_ID_M3FC_CFG_PYROS, false,
                 (uint8_t*)&m3fc_config.pyros, 8);

        m3status_set_ok(M3FC_COMPONENT_CFG);
        chThdSleepMilliseconds(5000);
    }
}

void m3fc_config_init() {
    m3status_set_init(M3FC_COMPONENT_CFG);
    chThdCreateStatic(m3fc_config_reporter_thd_wa,
                      sizeof(m3fc_config_reporter_thd_wa),
                      NORMALPRIO-10, m3fc_config_reporter_thd, NULL);
}
