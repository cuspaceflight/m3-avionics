#include "ch.h"
#include "hal.h"

#include "status.h"


static uint8_t sys_state = 2;
static uint8_t gps_state = 2;
static uint8_t pr_state = 2;
static uint8_t sr_state = 2;


void set_status(uint8_t component, uint8_t status) {

    switch (component) {
    
        case (COMPONENT_SYS):
            
            sys_state = status;
            
            break;
            
        case (COMPONENT_GPS):
            
            gps_state = status;
            
            break;
            
        case (COMPONENT_PR):
            
            pr_state = status;
            
            break;
            
        case (COMPONENT_SR):
            
            sr_state = status;
            
            break;
        
    }
}


static THD_WORKING_AREA(waStatusThread, 128);
static THD_FUNCTION(StatusThread, arg) {

    (void)arg;
    
    while(true) {
    
        switch(sys_state) {
        
            case (STATUS_GOOD):
        
                palSetPad(GPIOC, GPIOC_SYS_GD);
                palClearPad(GPIOC, GPIOC_SYS_ERR);
                
                break;
                
            case (STATUS_ERROR):
                
                palSetPad(GPIOC, GPIOC_SYS_ERR);
                palClearPad(GPIOC, GPIOC_SYS_GD);
                
                break;            
        }
        
        switch(gps_state) {
        
            case (STATUS_GOOD):
        
                palSetPad(GPIOA, GPIOA_GPS_GD);
                palClearPad(GPIOA, GPIOA_GPS_ERR);
                
                break;
            
            case (STATUS_ERROR):
                
                palSetPad(GPIOA, GPIOA_GPS_ERR);
                palClearPad(GPIOA, GPIOA_GPS_GD);
                
                break;              
        }
        
        switch(pr_state) {
        
            case (STATUS_GOOD):
        
                palSetPad(GPIOA, GPIOA_PR_GD);
                palClearPad(GPIOA, GPIOA_PR_ERR);
                
                break;
            
            case (STATUS_ERROR):
                
                palSetPad(GPIOA, GPIOA_PR_ERR);
                palClearPad(GPIOA, GPIOA_PR_GD);
                
                break;              
        }
        
        switch(sr_state) {
        
            case (STATUS_GOOD):
        
                palSetPad(GPIOA, GPIOA_SR_GD);
                palClearPad(GPIOA, GPIOA_SR_ERR);
                
                break;
            
            case (STATUS_ERROR):
                
                palSetPad(GPIOA, GPIOA_SR_ERR);
                palClearPad(GPIOA, GPIOA_SR_GD);
                
                break;              
        }
        
        chThdSleepMilliseconds(500);
    }
}


void status_init(void) {

    chThdCreateStatic(waStatusThread, sizeof(waStatusThread), NORMALPRIO, StatusThread, NULL);

}
