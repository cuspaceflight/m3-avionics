#ifndef SP100_H
#define SP100_H

#include "ch.h"
#include "hal.h"

struct SP100 {
    uint32_t id;
    uint16_t pressure;
    uint8_t acceleration, temperature, battery;
};

/* Initialise a thread to read `nss` SP100s from SS lines in `ss[]`,
 * storing results in `sp100s[]`.
 */
void sp100_init(SPIDriver* spid, size_t nss, ioline_t ss[],
                struct SP100 sp100s[]);

#endif
