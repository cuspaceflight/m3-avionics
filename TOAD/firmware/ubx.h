#ifndef __UBX_H__
#define __UBX_H__

/*
 * Private definitions of ubx packets
 */

/* UBX sync bytes */
#define UBX_SYNC1 0xB5
#define UBX_SYNC2 0x62

/* UBX Classes */
#define UBX_NAV 0x01
#define UBX_RXM 0x02
#define UBX_INF 0x04
#define UBX_ACK 0x05
#define UBX_CFG 0x06
#define UBX_UPD 0x09
#define UBX_MON 0x0A
#define UBX_AID 0x0B
#define UBX_TIM 0x0D
#define UBX_MGA 0x13
#define UBX_LOG 0x21
#define NMEA_CLASS 0xF0

/* Selection of UBX IDs */
#define UBX_CFG_PRT     0x00
#define UBX_CFG_MSG     0x01
#define UBX_CFG_RATE    0x08
#define UBX_CFG_SBAS    0x16
#define UBX_CFG_NAV5    0x24
#define UBX_CFG_TP5     0x31
#define UBX_CFG_GNSS    0x3E
#define UBX_NAV_POSECEF 0x01
#define UBX_NAV_PVT     0x07
#define NMEA_GGA 0x00
#define NMEA_GLL 0x01
#define NMEA_GSA 0x02
#define NMEA_GSV 0x03
#define NMEA_RMC 0x04
#define NMEA_VTG 0x05


/* UBX-CFG-PRT
 * Change port settings including protocols.
 */
typedef struct __attribute__((packed)) {
    uint8_t sync1, sync2, class, id;
    uint16_t length;
    union {
        uint8_t payload[20];
        struct {
            uint8_t port_id;
            uint8_t reserved0;
            uint16_t tx_ready;
            uint32_t mode;
            uint32_t baud_rate;
            uint16_t in_proto_mask;
            uint16_t out_proto_mask;
            uint16_t flags;
            uint16_t reserved5;
        } __attribute__((packed));
    };
    uint8_t ck_a, ck_b;
} ubx_cfg_prt_t;


/* UBX-CFG-MSG
 * Change rate (or disable) automatic delivery of messages
 * to the current port.
 */
typedef struct __attribute__((packed)) {
    uint8_t sync1, sync2, class, id;
    uint16_t length;
    union {
        uint8_t payload[3];
        struct {
            uint8_t msg_class;
            uint8_t msg_id;
            uint8_t rate;
        } __attribute__((packed));
    };
    uint8_t ck_a, ck_b;
} ubx_cfg_msg_t;


/* UBX-CFG-RATE
 * Change solution rate
 */
typedef struct __attribute__((packed)) {
    uint8_t sync1, sync2, class, id;
    uint16_t length;
    union {
        uint8_t payload[6];
        struct {
            uint16_t meas_rate;
            uint16_t nav_rate;
            uint16_t time_ref;
        } __attribute__((packed));
    };
    uint8_t ck_a, ck_b;
} ubx_cfg_rate_t;


/* UBX-CFG-SBAS
 * SBAS configuration
 */
typedef struct __attribute__((packed)) {
    uint8_t sync1, sync2, class, id;
    uint16_t length;
    union {
        uint8_t payload[8];
        struct {
            uint8_t mode;
            uint8_t usage;
            uint8_t max_sbas;
            uint8_t scanmode2;
            uint32_t scanmode1;
        } __attribute__((packed));
    };
    uint8_t ck_a, ck_b;
} ubx_cfg_sbas_t;

/* UBX-CFG-NAV5
 * Set navigation fix settings.
 */
typedef struct __attribute__((packed)){
    uint8_t sync1, sync2, class, id;
    uint16_t length;
    union {
        uint8_t payload[36];
        struct {
            uint16_t mask;
            uint8_t dyn_model;
            uint8_t fix_mode;
            int32_t fixed_alt;
            uint32_t fixed_alt_var;
            int8_t min_elev;
            uint8_t dr_limit;
            uint16_t p_dop, t_dop, p_acc, t_acc;
            uint8_t static_hold_thres;
            uint8_t dgps_timeout;
            uint8_t cno_thresh_num_svs, cno_thresh;
            uint16_t reserved;
            uint16_t static_hold_max_dist;
            uint8_t utc_standard;
            uint8_t reserved2;
            uint32_t reserved3;
        } __attribute__((packed));
    };
    uint8_t ck_a, ck_b;
} ubx_cfg_nav5_t;


/* UBX-CFG-TP5
 * Time Pulse Parameters
 */
typedef struct __attribute__((packed)) {
    uint8_t sync1, sync2, class, id;
    uint16_t length;
    union {
        uint8_t payload[32];
        struct {
            uint8_t tp_idx;
            uint8_t version;
            uint8_t reserved1[2];
            int16_t ant_cable_delay;
            int16_t rf_group_delay;
            uint32_t freq_period;
            uint32_t freq_period_lock;
            uint32_t pulse_len_ratio;
            uint32_t pulse_len_ratio_lock;
            int32_t user_config_delay;
            uint32_t flags;
        } __attribute__((packed));
    };
    uint8_t ck_a, ck_b;
} ubx_cfg_tp5_t;
// Flags for cfg-tp5
#define UBX_CFG_TP5_FLAGS_ACTIVE                (1<<0)
#define UBX_CFG_TP5_FLAGS_LOCK_GNSS_FREQ        (1<<1)
#define UBX_CFG_TP5_FLAGS_LOCKED_OTHER_SET      (1<<2)
#define UBX_CFG_TP5_FLAGS_IS_FREQ               (1<<3)
#define UBX_CFG_TP5_FLAGS_IS_LENGTH             (1<<4)
#define UBX_CFG_TP5_FLAGS_ALIGN_TO_TOW          (1<<5)
#define UBX_CFG_TP5_FLAGS_POLARITY              (1<<6)
#define UBX_CFG_TP5_FLAGS_GRID_UTC_GNSS_UTC     (0<<7)
#define UBX_CFG_TP5_FLAGS_GRID_UTC_GNSS_GPS     (1<<7)
#define UBX_CFG_TP5_FLAGS_GRID_UTC_GNSS_GLONASS (2<<7)
#define UBX_CFG_TP5_FLAGS_GRID_UTC_GNSS_BEIDOU  (3<<7)
#define UBX_CFG_TP5_FLAGS_GRID_UTC_GNSS_GALILEO (4<<7)


/* UBX_CFG_GNSS
 * GNSS system configuration
 */
 typedef struct __attribute__((packed)) {
     uint8_t sync1, sync2, class, id;
     uint16_t length;
     union {
         uint8_t payload[44];
         struct {
            uint8_t msg_ver;
            uint8_t num_trk_ch_hw;
            uint8_t num_trk_ch_use;
            uint8_t num_config_blocks;
            uint8_t gps_gnss_id;
            uint8_t gps_res_trk_ch;
            uint8_t gps_max_trk_ch;
            uint8_t gps_reserved1;
            uint32_t gps_flags;
            uint8_t sbas_gnss_id;
            uint8_t sbas_res_trk_ch;
            uint8_t sbas_max_trk_ch;
            uint8_t sbas_reserved1;
            uint32_t sbas_flags;
            uint8_t beidou_gnss_id;
            uint8_t beidou_res_trk_ch;
            uint8_t beidou_max_trk_ch;
            uint8_t beidou_reserved1;
            uint32_t beidou_flags;
            uint8_t qzss_gnss_id;
            uint8_t qzss_res_trk_ch;
            uint8_t qzss_max_trk_ch;
            uint8_t qzss_reserved1;
            uint32_t qzss_flags;
            uint8_t glonass_gnss_id;
            uint8_t glonass_res_trk_ch;
            uint8_t glonass_max_trk_ch;
            uint8_t glonass_reserved1;
            uint32_t glonass_flags;
         } __attribute__((packed));
     };
     uint8_t ck_a, ck_b;
 } ubx_cfg_gnss_t;


/* UBX-ACK
 * ACK/NAK messages after trying to set a config.
 */
typedef struct __attribute__((packed)) {
    uint8_t sync1, sync2, class, id;
    uint16_t length;
    union {
        uint8_t payload[2];
        struct {
            uint8_t cls_id;
            uint8_t msg_id;
        } __attribute__((packed));
    };
    uint8_t ck_a, ck_b;
} ubx_ack_t;


/* UBX-NAV-POSECEF
 * Position solution in ECEF
 */
typedef struct __attribute__((packed)) {
    uint8_t sync1, sync2, class, id;
    uint16_t length;
    union {
        uint8_t payload[20];
        struct {
            uint32_t i_tow;
            int32_t ecef_x, ecef_y, ecef_z;
            uint32_t p_acc;
        } __attribute__((packed));
    };
    uint8_t ck_a, ck_b;
} ubx_nav_posecef_t;


/* UBX-NAV-PVT
 * Contains fix quality, position and time information.
 * Everything you want in one message.
 */
typedef struct __attribute__((packed)) {
    uint8_t sync1, sync2, class, id;
    uint16_t length;
    union {
        uint8_t payload[92];
        struct {
            uint32_t i_tow;
            uint16_t year;
            uint8_t month, day, hour, minute, second;
            uint8_t valid;
            uint32_t t_acc;
            int32_t nano;
            uint8_t fix_type;
            uint8_t flags;
            uint8_t reserved1;
            uint8_t num_sv;
            int32_t lon, lat;
            int32_t height, h_msl;
            uint32_t h_acc, v_acc;
            int32_t velN, velE, velD, gspeed;
            int32_t head_mot;
            uint32_t s_acc;
            uint32_t head_acc;
            uint16_t p_dop;
            uint16_t reserved2;
            uint32_t reserved3;
            int32_t head_veh;
            uint32_t reserved4;
        } __attribute__((packed));
    };
    uint8_t ck_a, ck_b;
} ubx_nav_pvt_t;

#endif // __UBX_H__
