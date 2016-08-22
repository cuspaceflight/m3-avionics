#include "ch.h"
#include "hal.h"

int main(void) {

      halInit();
      chSysInit();

      palClearLine(LINE_FIRE1);
      palClearLine(LINE_FIRE2);
      palClearLine(LINE_FIRE3);
      palClearLine(LINE_FIRE4);

      while (true) {
      }
}
