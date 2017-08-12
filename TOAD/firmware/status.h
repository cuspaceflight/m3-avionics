#ifndef _STATUS_H
#define _STATUS_H

#define COMPONENT_SYS   0
#define COMPONENT_GPS   1
#define COMPONENT_PR    2
#define COMPONENT_SR    3  

#define STATUS_GOOD     1
#define STATUS_ERROR    2
#define STATUS_ACTIVITY 3 
   

/* Prototypes */
void status_init(void);
void set_status(uint8_t component, uint8_t status);

#endif
