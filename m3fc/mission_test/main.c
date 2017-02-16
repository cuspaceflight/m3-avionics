#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../firmware/m3fc_mission.c"
#include "../firmware/m3fc_state_estimation.c"

uint32_t current_time = 0;
struct m3fc_config m3fc_config = {
    .profile = {
        .m3fc_position = M3FC_CONFIG_POSITION_CORE,
        .accel_axis = M3FC_CONFIG_ACCEL_AXIS_Z,
        .ignition_accel = 20,
        .burnout_timeout = 30,
        .apogee_timeout = 10,
        .main_altitude = 2,
        .main_timeout = 30,
        .land_timeout = 30,
    },
};
enum m3fc_ui_beeper_mode m3fc_ui_beeper_mode = M3FC_UI_BEEPER_SLOW;

const char* state_names[] = {
    "init", "pad", "ignition", "powered ascent", "burnout",
    "free ascent", "apogee", "drogue descent", "release main",
    "main descent", "land", "landed"
};

struct log_packet {
    uint16_t sid;
    uint8_t rtr;
    uint8_t dlc;
    union {
        float    f32[2];
        int32_t  i32[2];
        uint32_t u32[2];
        int16_t  i16[4];
        uint16_t u16[4];
        int8_t    i8[8];
        uint8_t   u8[8];
    };
    uint32_t ts;
} __attribute__((packed));

int main(int argc, char* argv[])
{
    (void)mission_thread;

    if(argc != 2) {
        printf("Usage: %s <log file>\n", argv[0]);
        return 1;
    }

    FILE* logfile = fopen(argv[1], "rb");
    FILE* outfile = fopen("out.bin", "wb");
    struct log_packet packet;

    state_t cur_state = STATE_PAD;
    state_t new_state;
    instance_data_t data = {0};
    data.h_ground = -54.0f;
    systime_t last_mission_time = 0;

    while(fread(&packet, sizeof(struct log_packet), 1, logfile)) {
        /* set the global fake time */
        current_time = packet.ts;

        /* first time around, initialise timestamps */
        if(last_mission_time == 0) {
            last_mission_time = current_time;
            m3fc_state_estimation_init();
        }

        if(packet.sid == CAN_MSG_ID_M3FC_ACCEL) {
            /* convert accels to m/s/s floats and run SE */
            const float g = 9.80665f;
            float faccels[3] = {
                (float)packet.i16[0] * 0.0039 * g,
                (float)packet.i16[1] * 0.0039 * g,
                ((float)packet.i16[2]) * 0.00405 * g,
            };
            m3fc_state_estimation_new_accels(faccels, 156.96f, 10.01f);
            fwrite(&packet, sizeof(struct log_packet), 1, outfile);
        } else if(packet.sid == CAN_MSG_ID_M3FC_BARO) {
            /* run SE on new pressure */
            m3fc_state_estimation_new_pressure((float)packet.i32[1], 250.0f);
            fwrite(&packet, sizeof(struct log_packet), 1, outfile);
        } else if(packet.sid == CAN_MSG_ID_M3RADIO_GPS_ALT) {
            fwrite(&packet, sizeof(struct log_packet), 1, outfile);
        }

        if(current_time - last_mission_time > 100) {
            /* Run the mission state machine every 10ms */
            data.state = m3fc_state_estimation_get_state();
            new_state = run_state(cur_state, &data);
            if(new_state != cur_state) {
                printf("State change, old=%s new=%s\n",
                       state_names[cur_state], state_names[new_state]);
                printf("t=%d h=%f v=%f a=%f\n", current_time, data.state.h,
                       data.state.v, data.state.a);
                printf("\n");

                cur_state = new_state;
            }

            if(cur_state == STATE_APOGEE) {
                /* on the martlet junior flight, we are upside down after
                 * drogue release as the avionics hangs from the parachute.
                 */
                m3fc_config.profile.accel_axis = M3FC_CONFIG_ACCEL_AXIS_NZ;
            }

            struct log_packet se_t_h = {
                .sid = CAN_MSG_ID_M3FC_SE_T_H, .rtr = 0, .dlc = 8,
                .f32 = {0, data.state.h},
                .ts  = current_time,
            };
            struct log_packet se_v_a = {
                .sid = CAN_MSG_ID_M3FC_SE_V_A, .rtr = 0, .dlc = 8,
                .f32 = {data.state.v, data.state.a},
                .ts  = current_time,
            };
            struct log_packet se_var_h = {
                .sid = CAN_MSG_ID_M3FC_SE_VAR_H, .rtr = 0, .dlc = 8,
                .f32 = {p[0][0]},
                .ts = current_time,
            };
            struct log_packet se_var_v_a = {
                .sid = CAN_MSG_ID_M3FC_SE_VAR_V_A, .rtr = 0, .dlc = 8,
                .f32 = {p[1][1], p[2][2]},
                .ts = current_time,
            };
            struct log_packet mc_state = {
                .sid = CAN_MSG_ID_M3FC_MISSION_STATE, .rtr = 0, .dlc = 5,
                .u8 = {0, 0, 0, 0, new_state},
                .ts = current_time,
            };
            fwrite(&se_t_h,     sizeof(struct log_packet), 1, outfile);
            fwrite(&se_v_a,     sizeof(struct log_packet), 1, outfile);
            fwrite(&se_var_h,   sizeof(struct log_packet), 1, outfile);
            fwrite(&se_var_v_a, sizeof(struct log_packet), 1, outfile);
            fwrite(&mc_state,   sizeof(struct log_packet), 1, outfile);

            last_mission_time = current_time;
        }
    }

    fclose(logfile);
    fclose(outfile);

    return 0;
}
