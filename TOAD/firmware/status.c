#include "ch.h"
#include "hal.h"

#include "status.h"

/* State Variables - [Default = Error] */
static uint8_t sys_state = 2;
static uint8_t gps_state = 2;
static uint8_t pr_state = 2;
static uint8_t sr_state = 2;

/* Prototypes */
void update_state_LEDs(void);


/* Set Status of a Component */
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


/* Update LEDs */
void update_state_LEDs(void) {
    
    switch(sys_state) {
        
        case (STATUS_GOOD):
    
            palSetPad(GPIOC, GPIOC_SYS_GD);
            palClearPad(GPIOC, GPIOC_SYS_ERR);
            
            break;
            
        case (STATUS_ERROR):
            
            palSetPad(GPIOC, GPIOC_SYS_ERR);
            palClearPad(GPIOC, GPIOC_SYS_GD);
            
            break;   
        
        case (STATUS_ACTIVITY):
        
            palClearPad(GPIOC, GPIOC_SYS_GD);
            set_status(COMPONENT_SYS, STATUS_GOOD);
            
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
            
        case (STATUS_ACTIVITY):
        
            palClearPad(GPIOA, GPIOA_GPS_GD);
            set_status(COMPONENT_GPS, STATUS_GOOD);
            
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
            
        case (STATUS_ACTIVITY):
        
            palClearPad(GPIOA, GPIOA_PR_GD);
            set_status(COMPONENT_PR, STATUS_GOOD);
            
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
            
        case (STATUS_ACTIVITY):
        
            palClearPad(GPIOA, GPIOA_SR_GD);
            set_status(COMPONENT_SR, STATUS_GOOD);
            
            break;
                          
    }
}


/* Status Monitor Thread */
static THD_WORKING_AREA(waStatusThread, 128);
static THD_FUNCTION(StatusThread, arg) {

    (void)arg;
    chRegSetThreadName("STATUS");

    /* Main Loop */
    while(true) {

        update_state_LEDs();
        chThdSleepMilliseconds(200);
    }
}


/* Create Status Monitor Thread */
void status_init(void) {

    chThdCreateStatic(waStatusThread, sizeof(waStatusThread), NORMALPRIO, StatusThread, NULL);

}
