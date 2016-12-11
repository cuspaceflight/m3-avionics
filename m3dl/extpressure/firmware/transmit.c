#include "sp100.h"
#include "transmit.h"
#include <string.h>

void transmit_SP100(struct SP100 sp100res[]) {
    
    /* 8 Byte Results Buffer */
    uint16_t txres[4] = {0};
    
    /* Populate Buffer */
    txres[0] = sp100res[0].pressure;
    txres[1] = sp100res[1].pressure;
    txres[2] = sp100res[2].pressure;
    txres[3] = sp100res[3].pressure;
    
}
