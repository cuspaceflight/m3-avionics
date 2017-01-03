#ifndef DATALOGGING_H
#define DATALOGGING_H

/* Init Logging */
void logging_init(void);

/* Disable Logging */
void disable_logging(void);

/* Main Datalogging Thread */
void datalogging_thread(void* arg);

/* Log a CAN Packet */
void log_can(uint16_t ID, bool RTR, uint8_t len, uint8_t* data);

#endif
