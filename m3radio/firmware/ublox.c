/*
 * uBlox GPS receiver
 * 2014, 2016 Adam Greig, Cambridge University Spaceflight
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "ublox.h"
#include "ubx.h"
#include "ch.h"
#include "hal.h"
#include "m3can.h"
#include "m3radio_status.h"


/*Serial setup*/
SerialDriver* ublox_seriald;
static SerialConfig serial_cfg = {
    .speed = 9600,
    .cr1 = 0,
    .cr2 = 0,
    .cr3 = 0,
};


/* Config Flag */
static bool gps_configured = false;


static uint16_t ublox_fletcher_8(uint16_t chk, uint8_t *buf, uint8_t n);
static void ublox_checksum(uint8_t *buf);
static bool ublox_transmit(uint8_t *buf);
static enum ublox_result ublox_state_machine(uint8_t b);
static bool gps_configure(bool nav_pvt, bool nav_posecef, bool rising_edge);
static bool gps_tx_ack(uint8_t *buf);


binary_semaphore_t pvt_ready_sem;
ublox_pvt_t pvt_latest;


/* UBX Decoding State Machine States */
typedef enum {
    STATE_IDLE = 0, STATE_SYNC1, STATE_SYNC2,
    STATE_CLASS, STATE_ID, STATE_L1, STATE_L2,
    STATE_PAYLOAD, STATE_CK_A, NUM_STATES
} ubx_state;


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


/* Transmits a UBX message and blocks
 * until a ACK/NAK is recieved in response
 */
static bool gps_tx_ack(uint8_t *buf)
{
    if(!ublox_transmit(buf)) {
        return false;
    }

    enum ublox_result r;
    do {
        r = ublox_state_machine(sdGet(ublox_seriald));
    } while( (r != UBLOX_ACK) && (r != UBLOX_NAK) );

    if(r == UBLOX_NAK){
        m3status_set_error(M3RADIO_COMPONENT_UBLOX,
                           M3RADIO_ERROR_UBLOX_NAK);
        return false;
    }
    return true;
}


static void ublox_can_send_pvt(ublox_pvt_t *pvt) {
    m3can_send_i32(CAN_MSG_ID_M3RADIO_GPS_LATLNG, pvt->lat, pvt->lon, 2);
    m3can_send_i32(CAN_MSG_ID_M3RADIO_GPS_ALT, pvt->height, pvt->h_msl, 2);
    m3can_send_u8(CAN_MSG_ID_M3RADIO_GPS_TIME, pvt->year, pvt->year>>8,
                  pvt->month, pvt->day, pvt->hour, pvt->minute, pvt->second,
                  pvt->valid, 8);
    m3can_send_u8(CAN_MSG_ID_M3RADIO_GPS_STATUS, pvt->fix_type, pvt->flags,
                  pvt->num_sv, 0, 0, 0, 0, 0, 3);
}

/* Run new byte b through the UBX decoding state machine. Note that this
 * function preserves static state and processes new messages as appropriate
 * once received.
 */
//uint8_t rxbuf[255] = {0};
//uint8_t rxbufidx = 0;
static enum ublox_result ublox_state_machine(uint8_t b)
{
    //rxbuf[rxbufidx++] = b;
    static ubx_state state = STATE_IDLE;

    static uint8_t class, id;
    static uint16_t length;
    static uint16_t length_remaining;
    static uint8_t payload[128];
    static uint8_t ck_a, ck_b;
    static uint16_t ck;

    ubx_cfg_nav5_t cfg_nav5;
    ubx_nav_pvt_t nav_pvt;
    ubx_nav_posecef_t nav_posecef;
    ublox_posecef_t posecef;

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
                return UBLOX_RXLEN_TOO_LONG;
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
                state = STATE_IDLE;
                return UBLOX_BAD_CHECKSUM;
            }

            switch(class) {
                case UBX_ACK:
                    if(id == UBX_ACK_NAK) {
                        /* NAK */
                        m3status_set_error(M3RADIO_COMPONENT_UBLOX,
                                           M3RADIO_ERROR_UBLOX_NAK);
                        return UBLOX_NAK;
                    } else if(id == UBX_ACK_ACK) {
                        /* ACK */
                        /* No need to do anything */
                        return UBLOX_ACK;
                    } else {
                        m3status_set_error(M3RADIO_COMPONENT_UBLOX,
                                           M3RADIO_ERROR_UBLOX_DECODE);
                        return UBLOX_UNHANDLED;
                    }
                    break;
                case UBX_NAV:
                    if(id == UBX_NAV_PVT) {
                        /* PVT */
                        memcpy(nav_pvt.payload, payload, length);
                        memcpy(&pvt_latest, payload, length);

                        ublox_can_send_pvt(&pvt_latest);

                        /* Signal NAV-PVT Ready Semaphore */
	                    chBSemSignal(&pvt_ready_sem);

                        m3status_set_ok(M3RADIO_COMPONENT_UBLOX);
                        return UBLOX_NAV_PVT;
                    } else if(id == UBX_NAV_POSECEF){

                        /* Extract NAV-POSECEF Payload */
                        memcpy(nav_posecef.payload, payload, length);
                        memcpy(&posecef, payload, length);

                        m3status_set_ok(M3RADIO_COMPONENT_UBLOX);
                        return UBLOX_NAV_POSECEF;
                    } else {
                        m3status_set_error(M3RADIO_COMPONENT_UBLOX,
                                           M3RADIO_ERROR_UBLOX_DECODE);
                        return UBLOX_UNHANDLED;
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
                        return UBLOX_CFG_NAV5;
                    } else {
                        m3status_set_error(M3RADIO_COMPONENT_UBLOX,
                                           M3RADIO_ERROR_UBLOX_DECODE);
                        return UBLOX_UNHANDLED;
                    }
                    break;
                default:
                    return UBLOX_UNHANDLED;
            }
            break;

        default:
            state = STATE_IDLE;
            m3status_set_error(M3RADIO_COMPONENT_UBLOX,
                               M3RADIO_ERROR_UBLOX_DECODE);
            return UBLOX_ERROR;

    }
    return UBLOX_WAIT;
}

static bool gps_configure(bool nav_pvt, bool nav_posecef, bool rising_edge)
{
    ubx_cfg_prt_t prt;
    ubx_cfg_nav5_t nav5;
    ubx_cfg_msg_t msg;
    ubx_cfg_msg_t msg2;
    ubx_cfg_rate_t rate;
    ubx_cfg_sbas_t sbas;
    ubx_cfg_gnss_t gnss;
    ubx_cfg_tp5_t tp5_1;
    ubx_cfg_tp5_t tp5_2;
    gps_configured = true;

    /* Disable NMEA on UART */
    prt.sync1 = UBX_SYNC1;
    prt.sync2 = UBX_SYNC2;
    prt.class = UBX_CFG;
    prt.id = UBX_CFG_PRT;
    prt.length = sizeof(prt.payload);;
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

    gps_configured &= ublox_transmit((uint8_t*)&prt);
    if(!gps_configured) return false;

    /* Wait for it to stop barfing NMEA */
    chThdSleepMilliseconds(300);

    /* Clear the read buffer */
    while(sdGetTimeout(ublox_seriald, TIME_IMMEDIATE) != Q_TIMEOUT);

    /* Disable non GPS systems */
    gnss.sync1 = UBX_SYNC1;
    gnss.sync2 = UBX_SYNC2;
    gnss.class = UBX_CFG;
    gnss.id = UBX_CFG_GNSS;
    gnss.length = sizeof(gnss.payload);

    gnss.msg_ver = 0;
    gnss.num_trk_ch_hw = 32;
    gnss.num_trk_ch_use = 32;
    gnss.num_config_blocks = 5;

    /* Enable GPS, use all-1 channels */
    gnss.gps_gnss_id = 0;
    gnss.gps_res_trk_ch = 31;
    gnss.gps_max_trk_ch = 31;
    gnss.gps_flags = 1+(1<<16);

    /* Enable QZSS as per protocol spec */
    gnss.qzss_gnss_id = 5;
    gnss.qzss_res_trk_ch = 1;
    gnss.qzss_max_trk_ch = 1;
    gnss.qzss_flags = 1+(1<<16);

    /* Leave all other GNSS systems disabled */
    gnss.sbas_gnss_id = 1;
    gnss.beidou_gnss_id = 3;
    gnss.glonass_gnss_id = 6;
    gps_configured &= gps_tx_ack((uint8_t*)&gnss);
    if(!gps_configured) return false;


    /* Wait for reset */
    chThdSleepMilliseconds(500);

    /* Re-disable NMEA Output */
    gps_configured &= ublox_transmit((uint8_t*)&prt);
    if(!gps_configured) return false;

    /* Wait for it to stop barfing NMEA */
    chThdSleepMilliseconds(300);

    /* Clear the read buffer */
    while(sdGetTimeout(ublox_seriald, TIME_IMMEDIATE) != Q_TIMEOUT);

    /* Set to Airborne <4g dynamic mode */
    nav5.sync1 = UBX_SYNC1;
    nav5.sync2 = UBX_SYNC2;
    nav5.class = UBX_CFG;
    nav5.id = UBX_CFG_NAV5;
    nav5.length = sizeof(nav5.payload);;

    nav5.mask = 1 | (1<<10);
    nav5.dyn_model = 8;
    nav5.utc_standard = 3;  // USNO

    gps_configured &= gps_tx_ack((uint8_t*)&nav5);
    if(!gps_configured) return false;


    /* Set solution rate to 10Hz */
    rate.sync1 = UBX_SYNC1;
    rate.sync2 = UBX_SYNC2;
    rate.class = UBX_CFG;
    rate.id = UBX_CFG_RATE;
    rate.length = sizeof(rate.payload);

    rate.meas_rate = 100;
    rate.nav_rate = 1;
    rate.time_ref = 0;  // UTC
    gps_configured &= gps_tx_ack((uint8_t*)&rate);
    if(!gps_configured) return false;


    /* Disable sbas */
    sbas.sync1 = UBX_SYNC1;
    sbas.sync2 = UBX_SYNC2;
    sbas.class = UBX_CFG;
    sbas.id = UBX_CFG_SBAS;
    sbas.length = sizeof(sbas.payload);
    sbas.mode = 0;

    gps_configured &= gps_tx_ack((uint8_t*)&sbas);
    if(!gps_configured) return false;


    /* Set up 1MHz timepulse on TIMEPULSE pin*/
    tp5_1.sync1 = UBX_SYNC1;
    tp5_1.sync2 = UBX_SYNC2;
    tp5_1.class = UBX_CFG;
    tp5_1.id = UBX_CFG_TP5;
    tp5_1.length = sizeof(tp5_1.payload);

    tp5_1.tp_idx =           0;                 // TIMEPULSE
    tp5_1.version =          0;
    tp5_1.ant_cable_delay =  0;
    tp5_1.freq_period =      1000000;           // 1MHz
    tp5_1.pulse_len_ratio =  0xffffffff >> 1;   // (2^32/2)/2^32 = 50% duty cycle
    tp5_1.freq_period_lock = 1000000;
    tp5_1.pulse_len_ratio_lock = 0xffffffff >> 1;
    tp5_1.user_config_delay = 0;
    tp5_1.flags = (
        UBX_CFG_TP5_FLAGS_ACTIVE                    |
        UBX_CFG_TP5_FLAGS_LOCK_GNSS_FREQ            |
        UBX_CFG_TP5_FLAGS_IS_FREQ                   |
        UBX_CFG_TP5_FLAGS_ALIGN_TO_TOW              |
        UBX_CFG_TP5_FLAGS_POLARITY                  |
        UBX_CFG_TP5_FLAGS_GRID_UTC_GNSS_UTC);

    gps_configured &= gps_tx_ack((uint8_t*)&tp5_1);
    if(!gps_configured) return false;


    /* Set up 1Hz pulse on SAFEBOOT pin */
    tp5_2.sync1 = UBX_SYNC1;
    tp5_2.sync2 = UBX_SYNC2;
    tp5_2.class = UBX_CFG;
    tp5_2.id = UBX_CFG_TP5;
    tp5_2.length = sizeof(tp5_2.payload);

    tp5_2.tp_idx               = 1;     // Safeboot pin
    tp5_2.version              = 0;
    tp5_2.ant_cable_delay      = 0;
    tp5_2.freq_period          = 1;
    tp5_2.pulse_len_ratio      = 10000; // us
    tp5_2.freq_period_lock     = 1;
    tp5_2.pulse_len_ratio_lock = 10000;

    if(rising_edge) {

        /* Rising edge on top of second */
        tp5_2.flags = (
            UBX_CFG_TP5_FLAGS_ACTIVE                    |
            UBX_CFG_TP5_FLAGS_LOCK_GNSS_FREQ            |
            UBX_CFG_TP5_FLAGS_LOCKED_OTHER_SET          |
            UBX_CFG_TP5_FLAGS_IS_FREQ                   |
            UBX_CFG_TP5_FLAGS_IS_LENGTH                 |
            UBX_CFG_TP5_FLAGS_ALIGN_TO_TOW              |
            UBX_CFG_TP5_FLAGS_POLARITY                  |
            UBX_CFG_TP5_FLAGS_GRID_UTC_GNSS_UTC);
    } else {

        /* Falling edge on top of second */
        tp5_2.flags = (
            UBX_CFG_TP5_FLAGS_ACTIVE                    |
            UBX_CFG_TP5_FLAGS_LOCK_GNSS_FREQ            |
            UBX_CFG_TP5_FLAGS_LOCKED_OTHER_SET          |
            UBX_CFG_TP5_FLAGS_IS_FREQ                   |
            UBX_CFG_TP5_FLAGS_IS_LENGTH                 |
            UBX_CFG_TP5_FLAGS_ALIGN_TO_TOW              |
            UBX_CFG_TP5_FLAGS_GRID_UTC_GNSS_UTC);
    }

    gps_configured &= gps_tx_ack((uint8_t*)&tp5_2);
    if(!gps_configured) return false;


    /* Enable NAV PVT messages */
    if(nav_pvt){
        msg.sync1 = UBX_SYNC1;
        msg.sync2 = UBX_SYNC2;
        msg.class = UBX_CFG;
        msg.id = UBX_CFG_MSG;
        msg.length = sizeof(msg.payload);

        msg.msg_class = UBX_NAV;
        msg.msg_id    = UBX_NAV_PVT;
        msg.rate      = 1;
        gps_configured &= gps_tx_ack((uint8_t*)&msg);
        if(!gps_configured) return false;
    }


    /* Enable NAV POSECEF messages */
    if (nav_posecef){
        msg2.sync1 = UBX_SYNC1;
        msg2.sync2 = UBX_SYNC2;
        msg2.class = UBX_CFG;
        msg2.id = UBX_CFG_MSG;
        msg2.length = sizeof(msg2.payload);

        msg2.msg_class = UBX_NAV;
        msg2.msg_id    = UBX_NAV_POSECEF;
        msg2.rate      = 1;

        gps_configured &= gps_tx_ack((uint8_t*)&msg2);
        if(!gps_configured) return false;
    }


    return gps_configured;
}

static THD_WORKING_AREA(ublox_thd_wa, 512);
static THD_FUNCTION(ublox_thd, arg) {
    (void)arg;
    chRegSetThreadName("GPS");


    while(true) {
        if(gps_configured) {

            ublox_state_machine(sdGet(ublox_seriald));
        }
        else {

            m3status_set_error(M3RADIO_COMPONENT_UBLOX, M3RADIO_ERROR_UBLOX_CFG);

            break;
        }
    }
}

void ublox_init(SerialDriver* seriald, bool nav_pvt, bool nav_posecef,
                bool rising_edge) {

    m3status_set_init(M3RADIO_COMPONENT_UBLOX);
    ublox_seriald = seriald;

    /* We'll reset the uBlox so it's in a known state */
    palClearLine(LINE_GPS_RESET_N);
    chThdSleepMilliseconds(300);
    palSetLine(LINE_GPS_RESET_N);
    chThdSleepMilliseconds(500);

    sdStart(ublox_seriald, &serial_cfg);

    while(!gps_configure(nav_pvt, nav_posecef, rising_edge)) {
        m3status_set_error(M3RADIO_COMPONENT_UBLOX, M3RADIO_ERROR_UBLOX_CFG);
        chThdSleepMilliseconds(1000);
    }
    m3status_set_ok(M3RADIO_COMPONENT_UBLOX);
    return;
}

/* Init GPS Thread */
void ublox_thd_init(void){
    chThdCreateStatic(ublox_thd_wa, sizeof(ublox_thd_wa), NORMALPRIO, ublox_thd, NULL);
}
