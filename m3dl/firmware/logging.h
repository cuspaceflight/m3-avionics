
#ifndef DATALOGGING_H
#define DATALOGGING_H


/* Datalogger Init */

void logging_init(void);

void disable_logging(void);

/* 
 * Main datalogging thread that handles writing the data persistently.
 * It periodically fetches the data that is being logged by the below functions
 * and saves them to the microsd card.
 */

void datalogging_thread(void* arg);

void log_can(uint16_t ID, bool RTR, uint8_t len, uint8_t* data);

#endif
