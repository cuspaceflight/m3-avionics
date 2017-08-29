#include "ch.h"
#include "hal.h"
#include "m3radio_gps_ant.h"

void m3radio_gps_ant_init() {
    palSetLine(LINE_ANT_EN);
}
