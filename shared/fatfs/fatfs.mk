# FATFS files.
FATFSSRC = ${CHIBIOS}/os/various/fatfs_bindings/fatfs_diskio.c \
           ${CHIBIOS}/os/various/fatfs_bindings/fatfs_syscall.c \
           ../../shared/fatfs/src/ff.c \
           ../../shared/fatfs/src/option/unicode.c

FATFSINC = ../../shared/fatfs/src
