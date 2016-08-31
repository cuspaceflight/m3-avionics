#ifndef M3FC_STATUS_H
#define M3FC_STATUS_H
#include <stdbool.h>
#include "m3status.h"

#define M3FC_COMPONENT_MC           (1)
#define M3FC_COMPONENT_SE           (2)
#define M3FC_COMPONENT_CFG          (3)
#define M3FC_COMPONENT_UI_BEEPER    (4)
#define M3FC_COMPONENT_UI_LEDS      (5)
#define M3FC_COMPONENT_ACCEL        (6)
#define M3FC_COMPONENT_BARO         (7)
#define M3FC_COMPONENT_FLASH        (8)
#define M3FC_COMPONENT_MC_PYRO      (9)
#define M3FC_COMPONENT_MOCK         (10)

#define M3FC_ERROR_FLASH_CRC        (1)
#define M3FC_ERROR_FLASH_WRITE      (2)
#define M3FC_ERROR_CFG_READ         (3)
#define M3FC_ERROR_CFG_CHK_PROFILE  (8)
#define M3FC_ERROR_CFG_CHK_PYROS    (9)
#define M3FC_ERROR_ACCEL_BAD_ID     (10)
#define M3FC_ERROR_ACCEL_SELFTEST   (11)
#define M3FC_ERROR_ACCEL_TIMEOUT    (12)
#define M3FC_ERROR_ACCEL_AXIS       (13)
#define M3FC_ERROR_SE_PRESSURE      (14)
#define M3FC_ERROR_MC_PYRO_ARM      (15)
#define M3FC_ERROR_MC_PYRO_CONT     (4)
#define M3FC_ERROR_MC_PYRO_SUPPLY   (5)
#define M3FC_ERROR_MOCK_ENABLED     (16)
#define M3FC_ERROR_CAN_BAD_COMMAND  (17)

#endif
