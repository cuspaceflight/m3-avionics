#include "ch.h"
#include "m3status.h"
#include "m3can.h"

static uint8_t components[256] = {0};
static systime_t last_transmit[256] = {0};

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

    /* Check if we should transmit the new status */
    bool transmit_status = false;
    if(components[component] != status) {
        /* Transmit if status has changed since last time */
        transmit_status = true;
    } else if (ST2MS(chVTTimeElapsedSinceX(last_transmit[component])) > 500) {
        /* Transmit if we haven't sent this one in at least 500ms */
        transmit_status = true;
    }

    /* Store new status for this component */
    components[component] = status;

    if(transmit_status) {
        /* Compute overall status as highest of all components */
        uint8_t overall_status = m3status_get();

        uint8_t data[4] = {overall_status, component, status, errorcode};
        uint8_t len = 3;
        if(status == M3STATUS_ERROR) {
            len = 4;
        }

        can_send(m3can_own_id | CAN_MSG_ID_STATUS, false, data, len);
        last_transmit[component] = chVTGetSystemTimeX();
    }
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
    if(overall_status & M3STATUS_ERROR) {
        overall_status = M3STATUS_ERROR;
    }
    return overall_status;
}
