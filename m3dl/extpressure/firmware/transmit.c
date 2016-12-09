#include "sp100.h"
#include "transmit.h"
#include <string.h>

void transmit_SP100(struct SP100 sp100res) {
    
    /* 8 Byte Transmission Buffer */
    uint8_t txbuf[8];
    
    memset(txbuf, 0, 8);
    
    /* Populate Buffer */
    txbuf[0] = sp100res.id;
    txbuf[4] = sp100res.pressure;
    txbuf[6] = sp100res.acceleration;
    txbuf[7] = sp100res.temperature;
    
}
