#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "osal.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "microsd.h"
#include "config.h"
#include "status.h"

/* Toad Config */
toad_config toad;

/* Get Toad ID - (Blocking) */
void configure_toad(SDFS* file_system) {

    SDRESULT sderr;    
    SDFILE config_file;
    SDMODE read_mode = FA_READ;
    
    bool configured = false;
    
    uint8_t data[1] = {0};
        
    while(!configured) {    
        
        /* Attempt to open config file */
        sderr = microsd_open_file(&config_file, "toad.conf", read_mode, file_system);
        
        if(sderr != FR_OK) {
            set_status(COMPONENT_SYS, STATUS_ERROR);
            continue;
        }
        
        /* Attempt to read ID */
        sderr = microsd_read(&config_file, (char *)data, 1);
        
        if(sderr != FR_OK) {
            set_status(COMPONENT_SYS, STATUS_ERROR);
            continue;
        }
        
        /* Update ID */
        switch(data[0]) {
    
        case (49):
        
            toad.id = TOAD_1_ID;
            toad.delay = TOAD_1_DELAY;
            configured = true;                     
            break;
        
        case (50):
        
            toad.id = TOAD_2_ID;
            toad.delay = TOAD_2_DELAY;
            configured = true;                     
            break;
            
        case (51):
        
            toad.id = TOAD_3_ID;
            toad.delay = TOAD_3_DELAY;
            configured = true;                     
            break;
            
        case (52):
        
            toad.id = TOAD_4_ID;
            toad.delay = TOAD_4_DELAY;
            configured = true;                     
            break;
            
        case (53):
        
            toad.id = TOAD_5_ID;
            toad.delay = TOAD_5_DELAY;
            configured = true;                     
            break;
            
        case (54):
        
            toad.id = TOAD_MASTER_ID;
            configured = true;                     
            break;
        }
        
        microsd_close_file(&config_file);
    }
    
    /* Set global flag */
    toad.configured = true;   
}
