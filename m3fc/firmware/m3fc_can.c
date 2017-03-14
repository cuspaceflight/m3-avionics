#include "m3can.h"
#include "m3fc_status.h"
#include "m3fc_config.h"
#include "m3fc_ui.h"
#include "m3fc_mock.h"
#include "m3fc_mission.h"

void m3can_recv(uint16_t msg_id, bool rtr, uint8_t *data, uint8_t datalen) {
    (void)rtr;

    switch(msg_id) {
    case CAN_MSG_ID_M3PYRO_SUPPLY_STATUS:
        m3fc_mission_handle_pyro_supply(data, datalen);
        break;
    case CAN_MSG_ID_M3PYRO_ARM_STATUS:
        m3fc_mission_handle_pyro_arm(data, datalen);
        break;
    case CAN_MSG_ID_M3PYRO_CONTINUITY:
        m3fc_mission_handle_pyro_continuity(data, datalen);
        break;
    case CAN_MSG_ID_M3PSU_CHARGER_STATUS:
        m3fc_mission_handle_psu_charger_status(data, datalen);
        break;
    case CAN_MSG_ID_M3FC_SET_CFG_PROFILE:
        m3fc_config_handle_set_profile(data, datalen);
        break;
    case CAN_MSG_ID_M3FC_SET_CFG_PYROS:
        m3fc_config_handle_set_pyros(data, datalen);
        break;
    case CAN_MSG_ID_M3FC_SET_CFG_ACCEL_X:
        m3fc_config_handle_set_accel_cal_x(data, datalen);
        break;
    case CAN_MSG_ID_M3FC_SET_CFG_ACCEL_Y:
        m3fc_config_handle_set_accel_cal_y(data, datalen);
        break;
    case CAN_MSG_ID_M3FC_SET_CFG_ACCEL_Z:
        m3fc_config_handle_set_accel_cal_z(data, datalen);
        break;
    case CAN_MSG_ID_M3FC_SET_CFG_RADIO_FREQ:
        m3fc_config_handle_set_radio_freq(data, datalen);
        break;
    case CAN_MSG_ID_M3FC_SET_CFG_CRC:
        m3fc_config_handle_set_crc(data, datalen);
        break;
    case CAN_MSG_ID_M3FC_LOAD_CFG:
        m3fc_config_handle_load(data, datalen);
        break;
    case CAN_MSG_ID_M3FC_SAVE_CFG:
        m3fc_config_handle_save(data, datalen);
        break;
    case CAN_MSG_ID_M3FC_MOCK_ENABLE:
        m3fc_mock_handle_enable(data, datalen);
        break;
    case CAN_MSG_ID_M3FC_MOCK_ACCEL:
        m3fc_mock_handle_accel(data, datalen);
        break;
    case CAN_MSG_ID_M3FC_MOCK_BARO:
        m3fc_mock_handle_baro(data, datalen);
        break;
    case CAN_MSG_ID_M3FC_ARM:
        m3fc_mission_handle_arm(data, datalen);
        break;
    case CAN_MSG_ID_M3FC_FIRE:
        m3fc_mission_handle_fire(data, datalen);
        break;
    }
}
