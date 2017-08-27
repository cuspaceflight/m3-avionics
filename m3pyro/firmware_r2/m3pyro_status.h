#ifndef M3PYRO_STATUS_H
#define M3PYRO_STATUS_H

#include "m3status.h"

#define M3PYRO_COMPONENT_HAL        (1)
#define M3PYRO_COMPONENT_SELFTEST   (2)
#define M3PYRO_COMPONENT_CONTINUITY (3)
#define M3PYRO_COMPONENT_FIRING     (4)

#define M3PYRO_ERROR_ADC                    (1)
#define M3PYRO_ERROR_BADCH                  (2)
#define M3PYRO_ERROR_SELFTEST_DISCHARGE     (3)
#define M3PYRO_ERROR_SELFTEST_CONT          (4)
#define M3PYRO_ERROR_SELFTEST_1A            (5)
#define M3PYRO_ERROR_SELFTEST_3A            (6)
#define M3PYRO_ERROR_SELFTEST_SUPPLY        (7)
#define M3PYRO_ERROR_ESTOP                  (8)
#define M3PYRO_ERROR_FIRE_TYPE_UNKNOWN      (9)
#define M3PYRO_ERROR_FIRE_SUPPLY_UNKNOWN    (10)
#define M3PYRO_ERROR_FIRE_SUPPLY_FAULT      (11)
#define M3PYRO_ERROR_FIRE_BAD_MSG           (12)

/* Initialise M3Status and start LED driving threads. */
void m3pyro_status_init(void);

bool m3pyro_armed(void);
void m3pyro_arm(void);
void m3pyro_disarm(void);

#endif
