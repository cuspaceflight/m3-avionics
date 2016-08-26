#include <stdint.h>
#include <stdbool.h>
#include "microsd.h"
#include "hal.h"
#include "ff.h"
#include "chprintf.h"
#include "err_handler.h"

/* Function Prototypes */

static bool microsd_card_init(FATFS* fs);
static void microsd_card_try_init(FATFS* fs);
static void microsd_card_deinit(void);

/* SD Card Init */

static bool microsd_card_init(FATFS* fs)
{
    FRESULT sderr;

    /* Initialise the SDC interface */
    sdcStart(&SDCD1, NULL);

    /* Attempt to connect to the SD card */
    int i = sdcConnect(&SDCD1);
    if (i) {
        /* SD Card Connection Failure */ 
		err(0x03);
        return false;
    }

    /* Attempt to mount the filesystem */
    sderr = f_mount(fs, "A", 0);

    if(sderr != FR_OK){
		/* SD Card Mounting Failure */		
		err(0x04);
	};
    
    return sderr == FR_OK;
}

/* SD Card Init Retry */

static void microsd_card_try_init(FATFS* fs)
{
    while(!microsd_card_init(fs)) {
        microsd_card_deinit();
        chThdSleepMilliseconds(200);
    }
}

/* SD Card Disconnect */

static void microsd_card_deinit()
{
    /* Unmount FS */
    f_mount(0, "A", 0);

    /* Disconnect from card */
    sdcDisconnect(&SDCD1);

    /* Disable SDC peripheral */
    sdcStop(&SDCD1);
}

/* 
 * SD Card Open/Write Functions
 */

/* 
 * Open file in <path> to <fp>.
 * Inits SD card and mounts file system - Blocking (See try_init function).
 * ONLY ONE FILE CAN BE OPEN AT A TIME
 */

SDRESULT microsd_open_file(SDFILE* fp, const char* path, SDMODE mode,
    SDFS* sd)
{
    SDRESULT sderr;
    microsd_card_try_init(sd);
    sderr = f_open(fp, path, mode);
    if(sderr != FR_OK)
		/* SD Init Failed */        
		err(0x05);
    return sderr;
}

/* 
 * Open/create file using incremental naming scheme that follows the format
 * <filename>_<5-digit number>.<extension>.
 * E.g. if log_00001.bin exists, try log_00002.bin until we find one that
 * doesn't already exist or we reach the limit of 99999.
 */
SDRESULT microsd_open_file_inc(FIL* fp, const char* path, const char* ext,
    SDFS* sd)
{
    SDRESULT sderr;
    SDMODE mode = FA_WRITE | FA_CREATE_NEW;
    uint32_t file_idx = 0;
    char fname[25];

	/* Init SD Card */
    microsd_card_try_init(sd);

	/* Create Incremented File */
    while (true) {
        // try to open file with number file_idx
        file_idx++;
        chsnprintf(fname, 25, "%s_%05d.%s", path, file_idx, ext);
        sderr = f_open(fp, fname, mode);
        if (sderr == FR_EXIST) {
            continue;
        } else {
            if(sderr != FR_OK) {
				/* Incremental SD Init Failed */
                err(0x06);
			}
            return sderr;
        }
    }
}

/* 
 * Close file in <fp>.
 * Unmounts the file system and disconnects from the SD card as well.
 */

SDRESULT microsd_close_file(SDFILE* fp)
{
    SDRESULT sderr;
    sderr = f_close(fp);
    microsd_card_deinit();
    return sderr;
}

/* 
 * Write <btw> bytes from <buf> to <fp>.
 * Number of bytes written is currently not used for anything ...
 * If bytes_written < btw aftewards, disk is full.
 */

SDRESULT microsd_write(SDFILE* fp, const char* buf, unsigned int btw)
{
    SDRESULT sderr;
    unsigned int bytes_written;

    sderr = f_write(fp, (void*) buf, btw, &bytes_written);
    f_sync(fp);

    if(sderr != FR_OK) {
		err(0x07);
	}

    return sderr;
}


