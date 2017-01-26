/*
 * uBlox GPS receiver
 * 2014, 2016 Adam Greig, Cambridge University Spaceflight
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "ublox.h"
#include "ch.h"
#include "hal.h"
#include "m3can.h"
#include "m3radio_status.h"

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
#define UBX_CFG_PRT  0x00
#define UBX_CFG_MSG  0x01
#define UBX_CFG_NAV5 0x24
#define UBX_CFG_RATE 0x08
#define UBX_NAV_PVT  0x07
#define NMEA_GGA 0x00
#define NMEA_GLL 0x01
#define NMEA_GSA 0x02
#define NMEA_GSV 0x03
#define NMEA_RMC 0x04
#define NMEA_VTG 0x05

SerialDriver* ublox_seriald;

/* UBX Decoding State Machine States */
typedef enum {
    STATE_IDLE = 0, STATE_SYNC1, STATE_SYNC2,
    STATE_CLASS, STATE_ID, STATE_L1, STATE_L2,
    STATE_PAYLOAD, STATE_CK_A, NUM_STATES
} ubx_state;

/* Structs for various UBX messages */

/* UBX-CFG-NAV5
 * Set navigation fix settings.
 * Notably includes changing dynamic mode to "Airborne 4G".
 */
typedef struct __attribute__((packed)) {
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
            uint8_t reserved3;
            uint32_t reserved4;
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

static uint16_t ublox_fletcher_8(uint16_t chk, uint8_t *buf, uint8_t n);
static void ublox_checksum(uint8_t *buf);
static bool ublox_transmit(uint8_t *buf);
static void ublox_state_machine(uint8_t c);
static bool ublox_configure(void);

static SerialConfig serial_cfg = {
    .speed = 9600,
    .cr1 = 0,
    .cr2 = 0,
    .cr3 = 0,
};

/* Run the Fletcher-8 checksum, initialised to chk, over n bytes of buf */
static uint16_t ublox_fletcher_8(uint16_t chk, uint8_t *buf, uint8_t n)
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
static void ublox_checksum(uint8_t *buf)
{
    uint16_t plen;

    /* Check SYNC bytes are correct */
    if(buf[0] != UBX_SYNC1 && buf[1] != UBX_SYNC2)
        return;

    /* Extract payload length */
    plen = ((uint16_t*)buf)[2];

    uint16_t ck = ublox_fletcher_8(0, &buf[2], plen+4);

    /* Write new checksum to the buffer */
    buf[plen+6] = ck;
    buf[plen+7] = ck >> 8;
}

/* Transmit a UBX message over the Serial.
 * Message length is determined from the UBX length field.
 * Checksum is added automatically.
 */
static bool ublox_transmit(uint8_t *buf)
{
    size_t n, nwritten;
    systime_t timeout;

    /* Add checksum to outgoing message */
    ublox_checksum(buf);

    /* Determine length and thus suitable timeout in systicks (ms) */
    n = 8 + ((uint16_t*)buf)[2];
    timeout = MS2ST(n*2);

    /* Transmit message */
    nwritten = sdWriteTimeout(ublox_seriald, buf, n, timeout);
    if(nwritten != n) {
        m3status_set_error(M3RADIO_COMPONENT_UBLOX,
                           M3RADIO_ERROR_UBLOX_TIMEOUT);
    }

    return nwritten == n;
}

static void ublox_can_send_pvt(ublox_pvt_t *pvt) {
    can_send_i32(CAN_MSG_ID_M3RADIO_GPS_LATLNG, pvt->lat, pvt->lon, 2);
    can_send_i32(CAN_MSG_ID_M3RADIO_GPS_ALT, pvt->height, pvt->h_msl, 2);
    can_send_u8(CAN_MSG_ID_M3RADIO_GPS_TIME, pvt->year, pvt->year>>8,
                pvt->month, pvt->day, pvt->hour, pvt->minute, pvt->second,
                pvt->valid, 8);
    can_send_u8(CAN_MSG_ID_M3RADIO_GPS_STATUS, pvt->fix_type, pvt->flags,
                pvt->num_sv, 0, 0, 0, 0, 0, 3);
}

/* Run new byte b through the UBX decoding state machine. Note that this
 * function preserves static state and processes new messages as appropriate
 * once received.
 */
uint8_t rxbuf[255] = {0};
uint8_t rxbufidx = 0;
static void ublox_state_machine(uint8_t b)
{
    rxbuf[rxbufidx++] = b;
    static ubx_state state = STATE_IDLE;

    static uint8_t class, id;
    static uint16_t length;
    static uint16_t length_remaining;
    static uint8_t payload[128];
    static uint8_t ck_a, ck_b;
    static uint16_t ck;

    ubx_cfg_nav5_t cfg_nav5;
    ubx_nav_pvt_t nav_pvt;
    ublox_pvt_t pvt;

    switch(state) {
        case STATE_IDLE:
            if(b == UBX_SYNC1)
                state = STATE_SYNC1;
            break;

        case STATE_SYNC1:
            if(b == UBX_SYNC2)
                state = STATE_SYNC2;
            else
                state = STATE_IDLE;
            break;

        case STATE_SYNC2:
            class = b;
            state = STATE_CLASS;
            break;

        case STATE_CLASS:
            id = b;
            state = STATE_ID;
            break;

        case STATE_ID:
            length = (uint16_t)b;
            state = STATE_L1;
            break;

        case STATE_L1:
            length |= (uint16_t)b << 8;
            if(length >= 128) {
                m3status_set_error(M3RADIO_COMPONENT_UBLOX,
                                   M3RADIO_ERROR_UBLOX_DECODE);
                state = STATE_IDLE;
            }
            length_remaining = length;
            state = STATE_PAYLOAD;
            break;

        case STATE_PAYLOAD:
            if(length_remaining) {
                payload[length - length_remaining--] = b;
            } else {
                ck_a = b;
                state = STATE_CK_A;
            }
            break;

        case STATE_CK_A:
            ck_b = b;
            state = STATE_IDLE;

            /* verify checksum */
            ck = ublox_fletcher_8(0, &class, 1);
            ck = ublox_fletcher_8(ck, &id, 1);
            ck = ublox_fletcher_8(ck, (uint8_t*)&length, 2);
            ck = ublox_fletcher_8(ck, payload, length);
            if(ck_a != (ck&0xFF) || ck_b != (ck>>8)) {
                m3status_set_error(M3RADIO_COMPONENT_UBLOX,
                                   M3RADIO_ERROR_UBLOX_CHECKSUM);
                break;
            }

            switch(class) {
                case UBX_ACK:
                    if(id == 0x00) {
                        /* NAK */
                        m3status_set_error(M3RADIO_COMPONENT_UBLOX,
                                           M3RADIO_ERROR_UBLOX_NAK);
                    } else if(id == 0x01) {
                        /* ACK */
                        /* No need to do anything */
                    } else {
                        m3status_set_error(M3RADIO_COMPONENT_UBLOX,
                                           M3RADIO_ERROR_UBLOX_DECODE);
                    }
                    break;
                case UBX_NAV:
                    if(id == UBX_NAV_PVT) {
                        /* PVT */
                        memcpy(nav_pvt.payload, payload, length);
                        memcpy(&pvt, payload, length);

                        ublox_can_send_pvt(&pvt);

                        m3status_set_ok(M3RADIO_COMPONENT_UBLOX);
                    } else {
                        m3status_set_error(M3RADIO_COMPONENT_UBLOX,
                                           M3RADIO_ERROR_UBLOX_DECODE);
                    }
                    break;
                case UBX_CFG:
                    if(id == UBX_CFG_NAV5) {
                        /* NAV5 */
                        memcpy(cfg_nav5.payload, payload, length);
                        if(cfg_nav5.dyn_model != 8) {
                            m3status_set_error(M3RADIO_COMPONENT_UBLOX,
                                               M3RADIO_ERROR_UBLOX_FLIGHT_MODE);
                        }
                    } else {
                        m3status_set_error(M3RADIO_COMPONENT_UBLOX,
                                           M3RADIO_ERROR_UBLOX_DECODE);
                    }
                    break;
                default:
                    break;
            }
            break;

        default:
            state = STATE_IDLE;
            m3status_set_error(M3RADIO_COMPONENT_UBLOX,
                               M3RADIO_ERROR_UBLOX_DECODE);
            break;

    }
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
    nav5.sync1 = UBX_SYNC1;
    nav5.sync2 = UBX_SYNC2;
    nav5.class = UBX_CFG;
    nav5.id = UBX_CFG_NAV5;
    nav5.length = 36;

    nav5.mask = 1;
    nav5.dyn_model = 8;
    nav5.reserved3 = 0;
    nav5.reserved4 = 0;

    success &= ublox_transmit((uint8_t*)&nav5);
    if(!success) return false;

    /* Enable NAV UBX messages */
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

    /* Set solution rate to 10Hz */
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

    return success;
}

static THD_WORKING_AREA(ublox_thd_wa, 512);
static THD_FUNCTION(ublox_thd, arg) {
    (void)arg;
    /* We'll reset the uBlox so it's in a known state */
    palClearLine(LINE_GPS_RESET_N);
    chThdSleepMilliseconds(100);
    palSetLine(LINE_GPS_RESET_N);
    chThdSleepMilliseconds(500);

    sdStart(ublox_seriald, &serial_cfg);

    while(!ublox_configure()) {
        m3status_set_error(M3RADIO_COMPONENT_UBLOX, M3RADIO_ERROR_UBLOX_CFG);
        chThdSleepMilliseconds(1000);
    }

    while(true) {
        ublox_state_machine(sdGet(ublox_seriald));
    }
}

void ublox_init(SerialDriver* seriald) {
    m3status_set_init(M3RADIO_COMPONENT_UBLOX);
    ublox_seriald = seriald;
    chThdCreateStatic(ublox_thd_wa, sizeof(ublox_thd_wa), NORMALPRIO,
                      ublox_thd, NULL);
}
