#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef uint32_t systime_t;
extern uint32_t current_time;

#define ST2MS(x)                    (x/10)
#define ST2US(x)                    (x*100)
#define MS2ST(x)                    (x*10)
#define chVTGetSystemTime()         (current_time)
#define chVTGetSystemTimeX()        (current_time)
#define chVTTimeElapsedSinceX(x)    (current_time - x)

#define chThdSleepMilliseconds(x)
#define chThdCreateStatic(a, b, c, d, e) ((void)a)

#define THD_WORKING_AREA(x, y)      uint8_t x[y]
#define THD_FUNCTION(name, arg)     void name(void* arg)

typedef bool binary_semaphore_t;
#define chBSemWait(x)
#define chBSemSignal(x)
#define chBSemObjectInit(x, y)      ((void)x)
