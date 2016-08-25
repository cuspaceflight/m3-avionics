#include "m3can.h"
#include "m3fc_config.h"
#include "m3fc_ui.h"

void can_recv(uint16_t msg_id, bool can_rtr, uint8_t *data, uint8_t datalen) {
    (void)can_rtr;
    (void)datalen;

    if(msg_id == CAN_MSG_ID_M3PYRO_SUPPLY_STATUS) {
        if(data[0] > 40) {
            m3fc_ui_set_armed(true);
        } else {
            m3fc_ui_set_armed(false);
        }
    } else if(msg_id == CAN_MSG_ID_M3PYRO_CONTINUITY) {
        /* TODO process continuity results, use to set error condition */
    } else if(msg_id == CAN_MSG_ID_M3FC_SET_CFG_PROFILE) {
        m3fc_config.profile.m3fc_position   = data[0];
        m3fc_config.profile.accel_axis      = data[1];
        m3fc_config.profile.ignition_accel  = data[2];
        m3fc_config.profile.burnout_timeout = data[3];
        m3fc_config.profile.apogee_timeout  = data[4];
        m3fc_config.profile.main_altitude   = data[5];
        m3fc_config.profile.main_timeout    = data[6];
        m3fc_config.profile.land_timeout    = data[7];
    } else if(msg_id == CAN_MSG_ID_M3FC_SET_CFG_PYROS) {
        m3fc_config.pyros.pyro_1_usage      = data[0];
        m3fc_config.pyros.pyro_1_type       = data[1];
        m3fc_config.pyros.pyro_2_usage      = data[2];
        m3fc_config.pyros.pyro_2_type       = data[3];
        m3fc_config.pyros.pyro_3_usage      = data[4];
        m3fc_config.pyros.pyro_3_type       = data[5];
        m3fc_config.pyros.pyro_4_usage      = data[6];
        m3fc_config.pyros.pyro_4_type       = data[7];
    } else if(msg_id == CAN_MSG_ID_M3FC_LOAD_CFG) {
        m3fc_config_load();
    } else if(msg_id == CAN_MSG_ID_M3FC_SAVE_CFG) {
        m3fc_config_save();
    }
}
