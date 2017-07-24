
/*
 *TOAD GPS driver
 *CUSF 2017
 */

#include "gps.h"

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
#define UBX_CFG_NAV5    0x24
#define UBX_CFG_RATE    0x08
#define UBX_NAV_PVT     0x07
#define UBX_NAV_POSECEF 0x01
#define NMEA_GGA 0x00
#define NMEA_GLL 0x01
#define NMEA_GSA 0x02
#define NMEA_GSV 0x03
#define NMEA_RMC 0x04
#define NMEA_VTG 0x05

SerialDriver* gps_seriald;

// UBX message structs

/* UBX-CFG-NAV5
 * Set navigation fix settings.
 * "Stationary" mode.
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


static uint16_t gps_fletcher_8(uint16_t chk, uint8_t *buf, uint8_t n);
static void gps_checksum(uint8_t *buf);
static bool gps_transmit(uint8_t *buf);
static bool gps_configure(void);

static SerialConfig serial_cfg = {
    .speed = 9600,
    .cr1 = 0,
    .cr2 = 0,
    .cr3 = 0,
};


/* Run the Fletcher-8 checksum, initialised to chk, over n bytes of buf */
static uint16_t gps_fletcher_8(uint16_t chk, uint8_t *buf, uint8_t n)
{
    int i;
    uint8_t ck_a = chk & 0xff, ck_b = chk>>8;

    /* Run Fletcher-8 algorithm */
    for(i=0; i<n; i++) {
        ck_a += buf[i];
        ck_b += ck_a;
    }

    return (ck_b<<8) | (ck_a);
}

/* Computes the Fletcher-8 checksum over buf, using its length fields
 * to determine how much to read, returning the new checksum.
 */
static void gps_checksum(uint8_t *buf)
{
    uint16_t plen;

    /* Check SYNC bytes are correct */
    if(buf[0] != UBX_SYNC1 && buf[1] != UBX_SYNC2)
        return;

    /* Extract payload length */
    plen = ((uint16_t*)buf)[2];

    uint16_t ck = gps_fletcher_8(0, &buf[2], plen+4);

    /* Write new checksum to the buffer */
    buf[plen+6] = ck;
    buf[plen+7] = ck >> 8;
}

/* Transmit a UBX message over the Serial.
 * Message length is determined from the UBX length field.
 * Checksum is added automatically.
 */
static bool gps_transmit(uint8_t *buf)
{
    size_t n, nwritten;
    systime_t timeout;

    /* Add checksum to outgoing message */
    gps_checksum(buf);

    /* Determine length and thus suitable timeout in systicks (ms) */
    n = 8 + ((uint16_t*)buf)[2];
    timeout = MS2ST(n*2);

    /* Transmit message */
    nwritten = sdWriteTimeout(gps_seriald, buf, n, timeout);
    return nwritten == n;
}


static bool ublox_configure(void)
{
    ubx_cfg_prt_t prt;
    ubx_cfg_nav5_t nav5;
    ubx_cfg_msg_t msg;
    ubx_cfg_rate_t rate;
    bool success = true;

    /* Disable NMEA on UART */
    prt.sync1 = UBX_SYNC1;
    prt.sync2 = UBX_SYNC2;
    prt.class = UBX_CFG;
    prt.id = UBX_CFG_PRT;
    prt.length = 20;
    /* Program UART1 */
    prt.port_id = 1;
    /* Don't use TXReady GPIO */
    prt.tx_ready = 0;
    /* 8 bits, no polarity, 1 stop bit */
    prt.mode = (1<<4) | (3<<6) | (4<<9);
    /* 9600 baud */
    prt.baud_rate = 9600;
    /* only receive UBX protocol */
    prt.in_proto_mask = (1<<0);
    /* only send UBX protocol */
    prt.out_proto_mask = (1<<0);
    /* no weird timeout */
    prt.flags = 0;
    /* must be 0 */
    prt.reserved5 = 0;

    success &= ublox_transmit((uint8_t*)&prt);

    /* Set to Airborne <4g dynamic mode */
    !!!!!
    nav5.sync1 = UBX_SYNC1;
    nav5.sync2 = UBX_SYNC2;
    nav5.class = UBX_CFG;
    nav5.id = UBX_CFG_NAV5;
    nav5.length = 36;

    nav5.mask = 1;
    nav5.dyn_model = 8;
    nav5.reserved3 = 0;
    nav5.reserved4 = 0;
    !!!!!

    success &= ublox_transmit((uint8_t*)&nav5);
    if(!success) return false;

    /* Enable NAV UBX messages */
    !!!!!
    msg.sync1 = UBX_SYNC1;
    msg.sync2 = UBX_SYNC2;
    msg.class = UBX_CFG;
    msg.id = UBX_CFG_MSG;
    msg.length = 3;

    msg.msg_class = UBX_NAV;
    msg.msg_id    = UBX_NAV_PVT;
    msg.rate      = 1;
    success &= ublox_transmit((uint8_t*)&msg);
    if(!success) return FALSE;
    !!!!!

    /* Set solution rate to 10Hz */
    !!!!!
    rate.sync1 = UBX_SYNC1;
    rate.sync2 = UBX_SYNC2;
    rate.class = UBX_CFG;
    rate.id = UBX_CFG_RATE;
    rate.length = 6;

    rate.meas_rate = 100;
    rate.nav_rate = 1;
    rate.time_ref = 0;
    success &= ublox_transmit((uint8_t*)&rate);
    if(!success) return FALSE;
    !!!!!
    return success;

    //Other things to set e.g. second pulse
}


static THD_WORKING_AREA(gps_thd_wa, 512);
static THD_FUNCTION(gps_thd, arg) {
    (void)arg;
    /* We'll reset the uBlox so it's in a known state */
    palClearLine(GPS_RST);
    chThdSleepMilliseconds(100);
    palSetLine(GPS_RST);
    chThdSleepMilliseconds(500);

    sdStart(gps_seriald, &serial_cfg);

    while(!gps_configure()) {
        chThdSleepMilliseconds(1000);
    }

    while(true) {
        // remove this loop, make polling functions public instead?
    }
}


void gps_init(SerialDriver* seriald){
    gps_seriald = seriald;
    chThdCreateStatic(gps_thd_wa, sizeof(gps_thd_wa), NORMALPRIO,
    gps_thd, NULL);
}
