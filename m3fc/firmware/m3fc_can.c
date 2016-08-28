#include "m3can.h"
#include "m3fc_status.h"
#include "m3fc_config.h"
#include "m3fc_ui.h"
#include "m3fc_mock.h"

void can_recv(uint16_t msg_id, bool can_rtr, uint8_t *data, uint8_t datalen) {
    (void)can_rtr;
    (void)datalen;

    if(msg_id == CAN_MSG_ID_M3PYRO_SUPPLY_STATUS) {
        if(data[0] > 40) {
            m3fc_status_pyro_supply_good = true;
        } else {
            m3fc_status_pyro_supply_good = false;
        }
    } else if(msg_id == CAN_MSG_ID_M3PYRO_ARM_STATUS) {
        if(data[0]) {
            m3fc_status_pyro_armed = true;
        } else {
            m3fc_status_pyro_armed = false;
        }
    } else if(msg_id == CAN_MSG_ID_M3PYRO_CONTINUITY) {
        uint8_t pyro1_r = data[0];
        uint8_t pyro2_r = data[1];
        uint8_t pyro3_r = data[2];
        uint8_t pyro4_r = data[3];
        bool all_ok = true;
        if(m3fc_config.pyros.pyro_1_usage && pyro1_r > 100) {
            m3status_set_error(M3FC_COMPONENT_PYROS, M3FC_ERROR_PYROS_PYRO1);
            all_ok = false;
        }
        if(m3fc_config.pyros.pyro_2_usage && pyro2_r > 100) {
            m3status_set_error(M3FC_COMPONENT_PYROS, M3FC_ERROR_PYROS_PYRO2);
            all_ok = false;
        }
        if(m3fc_config.pyros.pyro_3_usage && pyro3_r > 100) {
            m3status_set_error(M3FC_COMPONENT_PYROS, M3FC_ERROR_PYROS_PYRO3);
            all_ok = false;
        }
        if(m3fc_config.pyros.pyro_4_usage && pyro4_r > 100) {
            m3status_set_error(M3FC_COMPONENT_PYROS, M3FC_ERROR_PYROS_PYRO4);
            all_ok = false;
        }
        if(all_ok) {
            m3status_set_ok(M3FC_COMPONENT_PYROS);
        }
    } else if(msg_id == CAN_MSG_ID_M3FC_SET_CFG_PROFILE) {
        m3fc_config.profile.m3fc_position   = data[0];
        m3fc_config.profile.accel_axis      = data[1];
        m3fc_config.profile.ignition_accel  = data[2];
        m3fc_config.profile.burnout_timeout = data[3];
        m3fc_config.profile.apogee_timeout  = data[4];
        m3fc_config.profile.main_altitude   = data[5];
        m3fc_config.profile.main_timeout    = data[6];
        m3fc_config.profile.land_timeout    = data[7];
        m3fc_config_check();
    } else if(msg_id == CAN_MSG_ID_M3FC_SET_CFG_PYROS) {
        m3fc_config.pyros.pyro_1_usage      = data[0];
        m3fc_config.pyros.pyro_2_usage      = data[1];
        m3fc_config.pyros.pyro_3_usage      = data[2];
        m3fc_config.pyros.pyro_4_usage      = data[3];
        m3fc_config.pyros.pyro_1_type       = data[4];
        m3fc_config.pyros.pyro_2_type       = data[5];
        m3fc_config.pyros.pyro_3_type       = data[6];
        m3fc_config.pyros.pyro_4_type       = data[7];
        m3fc_config_check();
    } else if(msg_id == CAN_MSG_ID_M3FC_LOAD_CFG) {
        m3fc_config_load();
        m3fc_config_check();
    } else if(msg_id == CAN_MSG_ID_M3FC_SAVE_CFG) {
        m3fc_config_save();
        m3fc_config_load();
        m3fc_config_check();
    } else if(msg_id == CAN_MSG_ID_M3FC_MOCK_ENABLE) {
        m3status_set_error(M3FC_COMPONENT_MOCK, M3FC_ERROR_MOCK_ENABLED);
        m3fc_mock_set_enabled();
    } else if(msg_id == CAN_MSG_ID_M3FC_MOCK_ACCEL) {
        uint16_t accels[3];
        accels[0] = data[0] | data[1]<<8;
        accels[1] = data[2] | data[3]<<8;
        accels[2] = data[4] | data[5]<<8;
        m3fc_mock_set_accel(accels[0], accels[1], accels[2]);
    } else if(msg_id == CAN_MSG_ID_M3FC_MOCK_BARO) {
        int32_t pressure, temperature;
        pressure = data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24;
        temperature = data[4] | data[5] << 8 | data[6] << 16 | data[7] << 24;
        m3fc_mock_set_baro(pressure, temperature);
    }
}
