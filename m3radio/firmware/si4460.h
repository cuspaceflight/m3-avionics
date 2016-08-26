#ifndef SI4460_H
#define SI4460_H

#include "hal.h"

void si4460_init(SPIDriver* spid, ioportid_t ssport, uint32_t sspad);

#endif
