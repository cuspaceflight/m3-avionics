#include "ch.h"
#include "hal.h"

#include "m3can.h"
#include "m3pyro_continuity.h"

int main(void) {

      halInit();
      chSysInit();

      can_init();

      palClearLine(LINE_FIRE1);
      palClearLine(LINE_FIRE2);
      palClearLine(LINE_FIRE3);
      palClearLine(LINE_FIRE4);

      m3pyro_continuity_init();

      while (true) {
      }
}

void can_recv(uint16_t msg_id, bool can_rtr, uint8_t *data, uint8_t datalen) {
    (void)msg_id;
    (void)can_rtr;
    (void)data;
    (void)datalen;
}
