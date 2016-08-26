#ifndef M3RADIO_STATUS_H
#define M3RADIO_STATUS_H

#include "m3status.h"

#define M3RADIO_COMPONENT_UBLOX         (1)
#define M3RADIO_COMPONENT_SI4460        (2)
#define M3RADIO_COMPONENT_GPSANT        (3)
#define M3RADIO_COMPONENT_PKTPROCESSOR  (4)

#define M3RADIO_ERROR_UBLOX_CHECKSUM    (1)
#define M3RADIO_ERROR_UBLOX_TIMEOUT     (2)
#define M3RADIO_ERROR_UBLOX_UARTERR     (3)
#define M3RADIO_ERROR_UBLOX_CFG         (4)
#define M3RADIO_ERROR_UBLOX_DECODE      (5)
#define M3RADIO_ERROR_UBLOX_FLIGHT_MODE (6)
#define M3RADIO_ERROR_UBLOX_NAK         (7)

void m3radio_status_init(void);

#endif
