#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "hal.h"
#include "microsd.h"
#include "logging.h"
#include "chprintf.h"
#include "osal.h"
#include "err_handler.h"

/* ------------------------------------------------------------------------- */
/* 				   DL PACKET    			     */
/* ------------------------------------------------------------------------- */

typedef struct DLPacket {

      	uint16_t ID;
	uint8_t RTR;
	uint8_t len;
	uint8_t data[8];
	systime_t timestamp;
} __attribute__((packed)) DLPacket;


/* ------------------------------------------------------------------------- */

#define LOG_MEMPOOL_ITEMS 3072  // 1K
#define LOG_CACHE_SIZE    16384 // 16KB

static THD_WORKING_AREA(logging_wa, 2048);

static void mem_init(void);
static void _log(DLPacket *packet);
void logging_init(void);



/* ------------------------------------------------------------------------- */
/* 				STATIC VARIABLES 			     */
/* ------------------------------------------------------------------------- */

/* Memory pool for allocating space for incoming data to be queued. */
static memory_pool_t log_mempool;

/* Mailbox (queue) for storing/accessing data asynchronously. Will be storing
 * pointers to data (packets).
 */
static mailbox_t log_mailbox;

/* Data cache to ensure SD writes are done LOG_CACHE_SIZE bytes at a time. */
static volatile char log_cache[LOG_CACHE_SIZE];

/* Statically allocated memory used for the memory pool */
static volatile char mempool_buffer[LOG_MEMPOOL_ITEMS * sizeof(DLPacket)]
                     __attribute__((aligned(sizeof(stkalign_t))))
                     __attribute__((section(".MAIN_STACK_RAM")));

/* Statically allocated memory used for the queue in mailbox */
static volatile msg_t mailbox_buffer[LOG_MEMPOOL_ITEMS]
                      __attribute__((section(".MAIN_STACK_RAM")));

static bool logging_enable = TRUE;


/* ------------------------------------------------------------------------- */
/* 			          ENTRY POINT	 			     */
/* ------------------------------------------------------------------------- */

void logging_init(void){

    /* Create Logging Thread */
    chThdCreateStatic(logging_wa, sizeof(logging_wa),
                      HIGHPRIO, datalogging_thread, NULL);
}


void disable_logging(void) {
    logging_enable = FALSE;
}



/* ------------------------------------------------------------------------- */
/* 			      MAIN THREAD FUNCTIONS 			     */
/* ------------------------------------------------------------------------- */

/* 
 * Main datalogging thread. Continuously checks for data added through the
 * logging functions, and persists it to an SD card
 */

THD_FUNCTION(datalogging_thread, arg) {

    static const int packet_size = sizeof(DLPacket);
    volatile char* cache_ptr = log_cache; // pointer to keep track of cache

    SDFS file_system;        // struct that encapsulates file system state
    SDFILE file;             // file struct thing
    msg_t mailbox_res;       // mailbox fetch result
    intptr_t data_msg;       // buffer to store the fetched mailbox item
    SDRESULT write_res;      // result of writing data to file system
    SDRESULT open_res;       // result of re-opening the log file
    (void)arg;

    /* Initialise Stuff */
    chRegSetThreadName("Datalogging");
    mem_init();

    while (microsd_open_file_inc(&file, "log", "bin", &file_system) != FR_OK);

    while (logging_enable) {

        /* Block waiting for a message to be available */
        mailbox_res = chMBFetch(&log_mailbox, (msg_t*)&data_msg, MS2ST(100));

        /* Mailbox was reset while waiting/fetch failed ... try again! */
        if (mailbox_res != MSG_OK || data_msg == 0) continue;

        /* Put packet in the static cache and free it from the memory pool */
        memcpy((void*)cache_ptr, (void*)data_msg, packet_size);
        chPoolFree(&log_mempool, (void*)data_msg);

        /* If the cache is full, write it all to the sd card */
        if(cache_ptr + packet_size >= log_cache + LOG_CACHE_SIZE) {
            write_res = microsd_write(&file, (char*)log_cache, LOG_CACHE_SIZE);

            /* If the write failed, keep attempting to re-open the log file
             * and write the data out when we succeed.
             */
            while (write_res != FR_OK) {
                err(0x08);
                microsd_close_file(&file);
                open_res = microsd_open_file_inc(&file, "log", "bin",
                                                 &file_system);
                if(open_res == FR_OK) {
                    write_res = microsd_write(&file, (char*)log_cache,
                                              LOG_CACHE_SIZE);
                }
            }

            /* Reset cache pointer to beginning of cache */
            cache_ptr = log_cache;
        } else {
            cache_ptr += packet_size;
        }
    }


    /* Flight Complete - Flush Cache to Disk and Unmount SD Card */
     
    write_res = microsd_write(&file, (char*)log_cache, (cache_ptr - log_cache));

    /* 
     * If the write failed, keep attempting to re-open the log file
     * and write the data out when we succeed.
     */

    while (write_res != FR_OK) {
        err(0x08);
        microsd_close_file(&file);
        open_res = microsd_open_file_inc(&file, "log", "bin",
                                         &file_system);
        if(open_res == FR_OK) {
            write_res = microsd_write(&file, (char*)log_cache,
                                      (cache_ptr - log_cache));
        }
    }
    
    microsd_close_file(&file);
}

/* 
 * Initialise memory management structures used to keep the data temporarily
 * in memory.
 */
static void mem_init(void)
{
    chMBObjectInit(&log_mailbox, (msg_t*)mailbox_buffer, LOG_MEMPOOL_ITEMS);
    chPoolObjectInit(&log_mempool, sizeof(DLPacket), NULL);

    /* 
     * Fill the memory pool with statically allocated bits of memory
     * ie. prevent dynamic core memory allocation (which cannot be freed), we
     * just want the "bookkeeping" that memory pools provide
     */
    chPoolLoadArray(&log_mempool, (void*)mempool_buffer, LOG_MEMPOOL_ITEMS);
}

/* ------------------------------------------------------------------------- */
/* 				         LOGGING CAN PACKET   		                         */
/* ------------------------------------------------------------------------- */


void log_can(uint16_t ID, bool RTR, uint8_t len, uint8_t* data)
{
    DLPacket pkt = {
        .ID = ID, .RTR = RTR,
        .len = len, .timestamp = chVTGetSystemTime()};
    memset(pkt.data,0,8);
    memcpy(pkt.data,data,len);
    _log(&pkt);
}



/* 
 * Allocate and post a formatted packet containing metadata + data to mailbox.
 * Use counter arrays to determine if this data should be sampled to radio.
 * (it's called _log because log conflicts with a library function)
 */
static void _log(DLPacket *packet)
{
    void* msg;
    msg_t retval;

    /* Allocate space for the packet and copy it into a mailbox message */
    msg = chPoolAlloc(&log_mempool);
    if (msg == NULL) return;
    memcpy(msg, (void*)packet, sizeof(DLPacket));

    /* put it in the mailbox buffer */
    retval = chMBPost(&log_mailbox, (intptr_t)msg, TIME_IMMEDIATE);
    if (retval != MSG_OK) {
        chPoolFree(&log_mempool, msg);
        return;
    }
}
