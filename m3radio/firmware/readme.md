STM32 ChibiOS Project
=====================

This is a sample project for running ChibiOS on an STM32. It has the bare minimum you need to get started on some generic hardware.

Setup:

1. You need to download ChibiOS itself and tell the Makefile where it is. Typically this means downloading a folder like ``ChibiOS_2.2.3`` and placing it in the current directory, then editing the makefile to point at it.
2. Edit board/board.h (and maybe board.c) to reflect the hardware you're running. Default port and direction values are specified here, defines for various hardware pins are specified here, clock values and the hardware name are specified here. Fill in the blanks.
3. Edit ch.ld to reflect your device's flash and ram sizes.
4. Edit chconf.h, halconf.h and mcuconf.h as appropriate for your application.
5. Write your code in main.c. A sample app that flashes some LEDs using the Mailbox synchronisation item is provided.

Put together by Adam Greig in 2011. All my contributions are public domain.

ChibiOS and the files from it used here are (C) 2006-2011 Giovanni Di Sirio and released under the GNU GPL v3 at time of writing.
