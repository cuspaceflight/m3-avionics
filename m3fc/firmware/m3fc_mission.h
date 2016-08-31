#ifndef MISSION_H
#define MISSION_H

extern volatile bool m3fc_mission_pyro_supply_good;
extern volatile bool m3fc_mission_pyro_armed;
extern volatile bool m3fc_mission_pyro_cont_ok;

void m3fc_mission_init(void);

void m3fc_mission_handle_arm(uint8_t* data, uint8_t datalen);
void m3fc_mission_handle_pyro_supply(uint8_t* data, uint8_t datalen);
void m3fc_mission_handle_pyro_arm(uint8_t* data, uint8_t datalen);
void m3fc_mission_handle_pyro_continuity(uint8_t* data, uint8_t datalen);

#endif
