#include "ch.h"
#include "hal.h"

#include "m3fc_ui.h"
#include "ms5611.h"

int main(void) {

    halInit();
    chSysInit();

    m3fc_ui_init();
    ms5611_init(&SPID1, GPIOC, GPIOC_BARO_CS);

    while (true) {
    }
}
