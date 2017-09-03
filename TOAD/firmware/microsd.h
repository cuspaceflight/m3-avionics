/* 
 * This is a non-thread-safe "device driver" for the micro sd card and file
 * system, pretty much acts as a file system access layer.
 *
 * Note: Assumes _LFN_UNICODE == 0 in FatFS config (ie. sizeof(TCHAR) == 1).
 */

#ifndef MICROSD_H
#define MICROSD_H

#include <stdio.h>
#include <string.h>

#include "integer.h"
#include "ffconf.h"
#include "ff.h"

/* FILE SYSTEM/SD CARD DATA TYPES */

/* SDMODE takes the value of BYTE, which are the possible file opening modes.
 * FA_READ for read access, FA_WRITE for write access, FA_CREATE_NEW to create
 * file that doesn't already exist, etc.
 * See FatFS documentation for all possible modes, or look in
 * ChibiOS-RT/ext/fatfs/src/ff.h.
 */

typedef BYTE SDMODE;

/* SDRESULT takes the values of FRESULT, which are return codes for file system
 * interaction. Will return FR_OK if successful. For errors local to this
 * (microsd) code, we use FR_INT_ERR by default.
 * See FatFS documentation for all possible return codes, or look in
 * ChibiOS-RT/ext/fatfs/src/ff.h.
 */

typedef FRESULT SDRESULT;

/* File Object */
typedef FIL SDFILE;

/* File System Object */
typedef FATFS SDFS;


/* FILE SYSTEM/SD CARD FUNCTIONS */

/* Open file in path <path> and opening mode <mode> to file object <fp>.
 * Blocking operation - Re-attempts indefinitely upon failure.
 * DO NOT open multiple files at once.
 */
 
SDRESULT microsd_open_file(SDFILE* fp, const char* path, SDMODE mode,
    SDFS* sd);

/* Open file under incremental naming scheme */
SDRESULT microsd_open_file_inc(SDFILE* fp, const char* path, const char* ext,
    SDFS* sd);

/* Close file object <fp> */
SDRESULT microsd_close_file(SDFILE* fp);

/* Assumes file is open.
 * Writes exactly <btw> bytes from <buff> to <fp>, or until disk is full.
 */
 
SDRESULT microsd_write(SDFILE* fp, const char* buff, unsigned int btw);

/* Assumes file is open.
 * Read <btr> Bytes into <buf> from <fp> 
 */
SDRESULT microsd_read(SDFILE* fp, const char* buf, uint32_t btr);

#endif
