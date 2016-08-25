#include "ch.h"
#include "m3status.h"
#include "m3can.h"

static uint8_t components[256] = {0};

static void m3status_set(uint8_t component, uint8_t status, uint8_t errorcode);

void m3status_set_ok(uint8_t component) {
    m3status_set(component, M3STATUS_OK, 0);
}

void m3status_set_init(uint8_t component) {
    m3status_set(component, M3STATUS_INITIALISING, 0);
}

void m3status_set_error(uint8_t component, uint8_t errorcode) {
    m3status_set(component, M3STATUS_ERROR, errorcode);
}

static void m3status_set(uint8_t component, uint8_t status, uint8_t errorcode)
{
    chDbgAssert(m3can_own_id != 0, "m3can_init() hasn't been called");

    /* Store new status for this component */
    components[component] = status;

    /* Compute overall status as highest of all components */
    uint8_t overall_status = m3status_get();

    uint8_t data[4] = {overall_status, component, status, errorcode};
    uint8_t len = 3;
    if(status == M3STATUS_ERROR) {
        len = 4;
    }

    can_send(m3can_own_id | CAN_MSG_ID_STATUS, false, data, len);
}

uint8_t m3status_get_component(uint8_t component) {
    return components[component];
}

uint8_t m3status_get() {
    uint8_t overall_status = 0;
    int i;
    for(i=0; i<256; i++) {
        overall_status |= components[i];
    }
    if(overall_status && M3STATUS_ERROR) {
        overall_status = M3STATUS_ERROR;
    }
    return overall_status;
}
