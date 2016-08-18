#include "ch.h"
#include "hal.h"

#include "m3fc_ui.h"
#include "ms5611.h"
#include "adxl345.h"

static const EXTConfig ext_cfg = {{
    {EXT_CH_MODE_DISABLED, NULL}, /* Pin 0 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Pin 1 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Pin 2 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Pin 3 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Pin 4 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Pin 5 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Pin 6 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Pin 7 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Pin 8 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Pin 9 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Pin 10 */
    {EXT_CH_MODE_AUTOSTART | EXT_CH_MODE_RISING_EDGE | EXT_MODE_GPIOA,
        adxl345_interrupt},       /* Pin 11 - PA11: ADXL345 INT1 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Pin 12 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Pin 13 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Pin 14 */
    {EXT_CH_MODE_DISABLED, NULL}, /* Pin 15 */
    {EXT_CH_MODE_DISABLED, NULL}, /* PVD */
    {EXT_CH_MODE_DISABLED, NULL}, /* RTC Alarm */
    {EXT_CH_MODE_DISABLED, NULL}, /* USB OTG FS Wakeup */
    {EXT_CH_MODE_DISABLED, NULL}, /* Ethernet Wakeup */
    {EXT_CH_MODE_DISABLED, NULL}, /* USB OTG HS Wakeup */
    {EXT_CH_MODE_DISABLED, NULL}, /* RTC Tamper/Timestamp */
    {EXT_CH_MODE_DISABLED, NULL}, /* RTC Wakeup */
}};

static const CANConfig can_cfg = {
    .mcr =
        /* Automatic Bus Off Management enabled,
         * Automatic Wake Up Management enabled.
         */
        CAN_MCR_ABOM | CAN_MCR_AWUM,
    .btr =
        /* CAN is on APB1 at 42MHz, we want 1Mbit/s.
         * 1/Baud = (BRP+1)/(APB1) * (3+TS1+TS2)
         */
        CAN_BTR_BRP(5) | CAN_BTR_TS1(3) | CAN_BTR_TS2(1)
        /* Allow up to 2 time quanta clock synchronisation */
        | CAN_BTR_SJW(1)
        /* XXX: Enable silent loopback test mode for now */
        /*| CAN_BTR_LBKM | CAN_BTR_SILM*/
};

static THD_WORKING_AREA(can_tx_wa, 256);
static THD_FUNCTION(can_tx_thd, arg) {
    (void)arg;

    CANTxFrame txmsg = {
        .IDE = CAN_IDE_STD,
        .RTR = CAN_RTR_DATA,
        .DLC = 8,
        .SID = 0b00100100011,
        .data32 = {
            0xDEADBEEF,
            0xCAFEBABE
        },
    };

    while(true) {
        canTransmit(&CAND1, CAN_ANY_MAILBOX, &txmsg, MS2ST(100));
        chThdSleepMilliseconds(500);
    }
}

static THD_WORKING_AREA(can_rx_wa, 256);
static THD_FUNCTION(can_rx_thd, arg) {
    (void)arg;

    event_listener_t el;
    CANRxFrame rxmsg;

    chEvtRegister(&CAND1.rxfull_event, &el, 0);

    while(true) {
        if(chEvtWaitAnyTimeout(ALL_EVENTS, MS2ST(100)) == 0) {
            continue;
        }

        while(canReceive(&CAND1, CAN_ANY_MAILBOX, &rxmsg,
                         TIME_IMMEDIATE) == MSG_OK) {
            palSetLine(LINE_LED_GRN);
            chThdSleepMilliseconds(50);
            palClearLine(LINE_LED_GRN);
        }
    }
}

int main(void) {

    halInit();
    chSysInit();

    m3fc_ui_init();
    ms5611_init(&SPID1, GPIOC, GPIOC_BARO_CS);
    adxl345_init(&SPID2, GPIOA, GPIOA_ACCEL_CS);

    extStart(&EXTD1, &ext_cfg);

    canStart(&CAND1, &can_cfg);

    chThdCreateStatic(can_rx_wa, sizeof(can_rx_wa), NORMALPRIO,
                      can_rx_thd, NULL);
    chThdCreateStatic(can_tx_wa, sizeof(can_tx_wa), NORMALPRIO,
                      can_tx_thd, NULL);

    while (true) {
    }
}
