/*
 * ch.h
 * Mocks/stubs for ChibiOS functionality.
 */


#ifndef CH_H
#define CH_H

#include <stdint.h>
#include <stddef.h>

#define ST2MS(X) (X)
#define NORMALPRIO (64)
#define THD_WORKING_AREA(s, n) uint32_t s[n / 4]
#define THD_FUNCTION(tname, arg) void tname(void *arg)

typedef uint32_t systime_t;

systime_t chVTGetSystemTimeX(void);
systime_t chVTTimeElapsedSinceX(systime_t);
void chThdSleepMilliseconds(uint32_t);
void chThdCreateStatic(void*, size_t, int, void (*)(void*), void*);

#endif
