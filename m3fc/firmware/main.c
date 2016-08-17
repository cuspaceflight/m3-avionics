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

int main(void) {

    halInit();
    chSysInit();

    m3fc_ui_init();
    ms5611_init(&SPID1, GPIOC, GPIOC_BARO_CS);
    adxl345_init(&SPID2, GPIOA, GPIOA_ACCEL_CS);

    extStart(&EXTD1, &ext_cfg);

    while (true) {
    }
}
