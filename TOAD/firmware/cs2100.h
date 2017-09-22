#ifndef LINETIME_CS2100_H
#define LINETIME_CS2100_H

#include "hal.h"

/* Configure the CS2100 to generate a suitable clock input.
 * Blocks until the CS2100 PLL indicates lock.
 */
void cs2100_configure(I2CDriver* i2cd);

/* Set up STM32 PLL to use CS2100 clock output.
 * Blocks until swapped to new clock.
 */
void cs2100_set_pll(void);

#endif
