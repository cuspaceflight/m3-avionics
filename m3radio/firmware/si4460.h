#ifndef SI4460_H
#define SI4460_H

#include "hal.h"

struct si4460_config {
    SPIDriver* spid;
    SPIConfig spi_cfg;
    bool sdn;
    ioline_t sdnline;
    bool tcxo;
    uint32_t xo_freq;
    uint32_t centre_freq;
    uint32_t data_rate;
    uint32_t deviation;
};

void si4460_init(struct si4460_config* config);

#endif
