#ifndef SI4460_H
#define SI4460_H

#include "hal.h"

extern binary_semaphore_t si4460_tx_sem;
extern uint8_t si4460_tx_buf[12];

void si4460_init(SPIDriver* spid, ioportid_t ssport, uint32_t sspad);

#endif
