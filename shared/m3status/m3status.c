#include "m3status.h"
#include "m3can.h"

static void m3status_set(uint8_t status, uint8_t errorcode);

void m3status_set_ok() {
    m3status_set(M3STATUS_OK, 0);
}

void m3status_set_initialising() {
    m3status_set(M3STATUS_INITIALISING, 0);
}

void m3status_set_error(uint8_t errorcode) {
    m3status_set(M3STATUS_ERROR, errorcode);
}

static void m3status_set(uint8_t status, uint8_t errorcode) {
    if(m3can_own_id == 0) {
        return;
    }

    uint8_t data[2] = {status, errorcode};
    uint8_t len = 1;
    if(status == M3STATUS_ERROR) {
        len = 2;
    }

    can_send(m3can_own_id | CAN_MSG_ID_STATUS, false, data, len);
}
