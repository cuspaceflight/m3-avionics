#include "hal.h"

#include "ff.h"
#include "chprintf.h"
#include <stdint.h>
#include <stdbool.h>

#include "microsd.h"
#include "status.h"

/* Function Prototypes */
static bool microsd_card_init(FATFS* fs);
static void microsd_card_try_init(FATFS* fs);
static void microsd_card_deinit(void);

/* Driver Working Area */
static uint8_t sd_scratchpad[512];

/* SD Card Config */
static const SDCConfig sdccfg = {
  sd_scratchpad,
  SDC_MODE_4BIT
};


/* SD Card Initilisation */
static bool microsd_card_init(FATFS* fs) {
    
    /* File System Return Code */
    FRESULT sderr;

    /* Initialise the SDC interface */
    sdcStart(&SDCD1, &sdccfg);

    /* Attempt to connect to the SD card */
    int i = sdcConnect(&SDCD1);
    if (i) {
        
        /* SD Card Connection Failure */ 
		set_status(COMPONENT_SYS, STATUS_ERROR);
        return false;
    }

    /* Attempt to mount the filesystem */
    sderr = f_mount(fs, "A", 0);
    if(sderr != FR_OK) {
		
		/* SD Card Mounting Failure */		
		set_status(COMPONENT_SYS, STATUS_ERROR);
	};
    
    /* Return TRUE */
    return (sderr == FR_OK);
}


/* Continually Re-attempt SD Card Initilisation */
static void microsd_card_try_init(FATFS* fs) {
      
    while(!microsd_card_init(fs)) {
        microsd_card_deinit();
        chThdSleepMilliseconds(200);
    }
}


/* SD Card Disconnect */
static void microsd_card_deinit() {
    
    /* Unmount File System */
    f_mount(0, "A", 0);

    /* Disconnect from card */
    sdcDisconnect(&SDCD1);

    /* Disable SDC peripheral */
    sdcStop(&SDCD1);
}


/* Open File in <path> to <fp> - Inits SD Card and Mounts File System */
SDRESULT microsd_open_file(SDFILE* fp, const char* path, SDMODE mode, SDFS* sd) {
    
    /* File System Return Code */
    SDRESULT sderr;
    
    /* Continually Re-attempt SD Card Initilisation */
    microsd_card_try_init(sd);
    
    /* Attempt to Open File */
    sderr = f_open(fp, path, mode);    
    if(sderr != FR_OK) {
    
		/* Failed to Open File */        
		set_status(COMPONENT_SYS, STATUS_ERROR);
    }
		
    return sderr;
}

/* 
 * Open/reate file using incremental naming scheme that follows 
 * the format <filename>_<5-digit number>.<extension>.
 * E.g. if log_00001.bin exists, try log_00002.bin until we find 
 * one that doesn't already exist or we reach the limit of 99999.
 */
 
SDRESULT microsd_open_file_inc(FIL* fp, const char* path, const char* ext, SDFS* sd) {
    
    /* File System Return Code */
    SDRESULT sderr;
    SDMODE mode = FA_WRITE | FA_CREATE_NEW;
    
    /* Buffer to Hold Filename */
    char fname[25];
    uint32_t file_idx = 0;

	/* Continually Re-attempt SD Card Initilisation */
    microsd_card_try_init(sd);
    
    while (true) {
    
        /* Attempt to Open File <fname>_<file_idx>.<ext> */
        file_idx++;
        chsnprintf(fname, 25, "%s_%05d.%s", path, file_idx, ext);
        sderr = f_open(fp, fname, mode);
        
        /* Existance Check */
        if (sderr == FR_EXIST) {
            continue;
        } else {
            if(sderr != FR_OK) {
            
				/* Failed to Open File */
                set_status(COMPONENT_SYS, STATUS_ERROR);                
			}
            return sderr;
        }
    }
}

/* Close File in <fp> - Unmounts File System ad Disconnects SD Card */
SDRESULT microsd_close_file(SDFILE* fp) {
    
    SDRESULT sderr;
    sderr = f_close(fp);
    microsd_card_deinit();
    return sderr;
}

/* Write <btw> Bytes From <buf> to <fp> */
SDRESULT microsd_write(SDFILE* fp, const char* buf, unsigned int btw) {
    
    SDRESULT sderr;
    unsigned int bytes_written;

    /* Write to SD Card */
    sderr = f_write(fp, (void*) buf, btw, &bytes_written);
    f_sync(fp);

    /* Test for SD Card Overflow */
	if(bytes_written < btw) {
	    set_status(COMPONENT_SYS, STATUS_ERROR);
	}

    /* Test for Succesful Write */
    if(sderr != FR_OK) {
		set_status(COMPONENT_SYS, STATUS_ERROR);
	}
	
    return sderr;
}

/* Read <btr> Bytes into <buf> from <fp> */
SDRESULT microsd_read(SDFILE* fp, const char* buf, uint32_t btr) {

    SDRESULT sderr;
    unsigned int bytes_read;
    
    /* Read from SD Card */
    sderr = f_read (fp, (void*) buf, btr, &bytes_read);	
    
    /* Test for success */
    	if(bytes_read < btr) {
	    set_status(COMPONENT_SYS, STATUS_ERROR);
	}

    /* Test for Succesful Write */
    if(sderr != FR_OK) {
		set_status(COMPONENT_SYS, STATUS_ERROR);
	}
	
	return sderr;
}    
    
