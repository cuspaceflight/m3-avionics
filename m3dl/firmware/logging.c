#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "osal.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "logging.h"
#include "microsd.h"
#include "LTC2983.h"
#include "logging.h"
#include "err_handler.h"

#include "m3can.h"
#include "m3status.h"

#define LOG_MEMPOOL_ITEMS 3072      // 1K
#define LOG_CACHE_SIZE    16384     // 16KB

/* Datalogger Packet */
typedef struct DLPacket {

    uint16_t ID;
	uint8_t RTR;
	uint8_t len;
	uint8_t data[8];
	systime_t timestamp;
} __attribute__((packed)) DLPacket;


/* Function Prototypes */
static void mem_init(void);
void logging_init(void);
static void _log(DLPacket *packet);


/* Logging Enabled/Disabled */
static bool logging_enable = TRUE;

/* Data cache to ensure SD writes are done LOG_CACHE_SIZE bytes at a time */
static volatile char log_cache[LOG_CACHE_SIZE];

/* Memory pool for allocating space for incoming data to be queued */
static memory_pool_t log_mempool;

/* Mailbox (queue) for storing pointers to data (packets) */
static mailbox_t log_mailbox;

/* Statically allocated memory used for the memory pool */
static volatile char mempool_buffer[LOG_MEMPOOL_ITEMS * sizeof(DLPacket)]
                     __attribute__((aligned(sizeof(stkalign_t))))
                     __attribute__((section(".MAIN_STACK_RAM")));

/* Statically allocated memory used for the queue in mailbox */
static volatile msg_t mailbox_buffer[LOG_MEMPOOL_ITEMS]
                      __attribute__((section(".MAIN_STACK_RAM")));


/* Datalogging Thread */
static THD_WORKING_AREA(logging_wa, 3072);
THD_FUNCTION(datalogging_thread, arg) {

    (void)arg;
    chRegSetThreadName("Datalogging");

    /* Packet Size */
    static const int packet_size = sizeof(DLPacket);
    
    /* Pointer to Keep Track of Cache */
    volatile char* cache_ptr = log_cache;
    
    /* File System Variables */
    SDFS file_system;
    FATFS *fsp;
    SDFILE file;
    SDRESULT write_res;
    SDRESULT open_res;
    
    /* Number of Avaliable 16KB Clusters */
    uint32_t free_clusters;    
    
    /* Mailbox Variables */             
    msg_t mailbox_res;       
    intptr_t data_msg;     

    /* Initalise Memory */
    mem_init();

    /* Attempt to Open log_xxxxx.bin */
    while (microsd_open_file_inc(&file, "log", "bin", &file_system) != FR_OK);
    
    /* SD Card Initilised and File Opened */
    m3status_set_ok(M3DL_COMPONENT_SD_CARD);

    /* Begin Logging */
    while (logging_enable) {

        /* Wait for Message to be Avaliable */
        mailbox_res = chMBFetch(&log_mailbox, (msg_t*)&data_msg, MS2ST(100));

        /* Re-attempt if Mailbox was Reset or Fetch Failed */
        if (mailbox_res != MSG_OK || data_msg == 0) continue;

        /* Put Packet in Static Cache and Free From Memory Pool */
        memcpy((void*)cache_ptr, (void*)data_msg, packet_size);
        chPoolFree(&log_mempool, (void*)data_msg);

        /* Detect Full Cache and Write to SD Card */
        if(cache_ptr + packet_size >= log_cache + LOG_CACHE_SIZE) {
            
            /* Attempt to Write Cache */
            write_res = microsd_write(&file, (char*)log_cache, LOG_CACHE_SIZE);
                             
            while (write_res != FR_OK) {
            
                /* Signal Failed Write */           
                err(M3DL_ERROR_SD_CARD_WRITE);
                m3status_set_error(M3DL_COMPONENT_SD_CARD, M3DL_ERROR_SD_CARD_WRITE);
                
                /* Attempt to Re-open File */
                microsd_close_file(&file);
                open_res = microsd_open_file_inc(&file, "log", "bin", &file_system);
                
                if(open_res == FR_OK) {
                    
                    /* Re-attempt to Write Cache */
                    write_res = microsd_write(&file, (char*)log_cache, LOG_CACHE_SIZE);
                }
            }

            /* Reset Cache Pointer */
            cache_ptr = log_cache;
                
            /* Report Free Space Over CAN */
            if(f_getfree("/", &free_clusters, &fsp) == FR_OK) {    
                can_send(CAN_MSG_ID_M3DL_FREE_SPACE, FALSE, (uint8_t*)(&free_clusters), 4);
            }
            
            /* Cache written to SD card succesfully */
            m3status_set_ok(M3DL_COMPONENT_SD_CARD);            
        
        } else {
        
            /* Increment Cache Pointer */
            cache_ptr += packet_size;
        }
    }


    /* Logging Disabled - Attempt to Flush Remainder of Cache to Disk */
    write_res = microsd_write(&file, (char*)log_cache, (cache_ptr - log_cache));

    while (write_res != FR_OK) {

        /* Signal Failed Write */           
        err(M3DL_ERROR_SD_CARD_WRITE);
        m3status_set_error(M3DL_COMPONENT_SD_CARD, M3DL_ERROR_SD_CARD_WRITE);

        /* Attempt to Re-open File */
        microsd_close_file(&file);
        open_res = microsd_open_file_inc(&file, "log", "bin", &file_system);

        if(open_res == FR_OK) {
            
            /* Re-attempt to Write Cache */
            write_res = microsd_write(&file, (char*)log_cache, (cache_ptr - log_cache));
        }
    }
    
    /* Cache written to SD card succesfully */
    m3status_set_ok(M3DL_COMPONENT_SD_CARD);
    
    /* Close File and Disconnect From SD Card */
    microsd_close_file(&file);
}

/* Initialise Mailbox and Memorypool */
static void mem_init(void) {
    
    chMBObjectInit(&log_mailbox, (msg_t*)mailbox_buffer, LOG_MEMPOOL_ITEMS);
    chPoolObjectInit(&log_mempool, sizeof(DLPacket), NULL);

    /* Fill Memory Pool with Statically Allocated Bits of Memory */
    chPoolLoadArray(&log_mempool, (void*)mempool_buffer, LOG_MEMPOOL_ITEMS);
}


/* Allocate and Post a Formatted Packet to the Mailbox */
static void _log(DLPacket *packet) {

    void* msg;
    msg_t retval;

    /* Allocate Space for Packet and Copy it into a Mailbox Message */
    msg = chPoolAlloc(&log_mempool);
    if (msg == NULL) return;
    memcpy(msg, (void*)packet, sizeof(DLPacket));

    /* Put it in the Mailbox Buffer */
    retval = chMBPost(&log_mailbox, (intptr_t)msg, TIME_IMMEDIATE);
    if (retval != MSG_OK) {
        chPoolFree(&log_mempool, msg);
        return;
    }
}


/* Init Logging */
void logging_init(void) {

    /* Create Datalogging Thread */
    chThdCreateStatic(logging_wa, sizeof(logging_wa),
                      HIGHPRIO, datalogging_thread, NULL);
}


/* Disable Logging */
void disable_logging(void) {
    logging_enable = FALSE;
}


/* Log a CAN Packet */
void log_can(uint16_t ID, bool RTR, uint8_t len, uint8_t* data) {
    
    DLPacket pkt = {
        .ID = ID, .RTR = RTR,
        .len = len, .timestamp = chVTGetSystemTime()};
    memset(pkt.data,0,8);
    memcpy(pkt.data,data,len);
    _log(&pkt);
}


