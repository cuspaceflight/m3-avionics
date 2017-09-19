#include "ch.h"
#include "hal.h"
#include "m3pyro_status.h"
#include "m3pyro_hal.h"
#include "m3pyro_continuity.h"

/* Configure fire times in ms. */
#define EMATCH_FIRE_TIME_MS     (1500)
#define TALON_FIRE_TIME_MS      (3000)
#define METRON_FIRE_TIME_MS     (500)

/* The fire command byte is split into two nibbles:
 * upper 4 bits: 0000 for 1A, 0001 for 3A.
 * lower 4 bits: pyrotechnic type:
 *      0 for none, 1 for ematch, 2 for talon, 3 for metron
 */
#define FIRE_TYPE_MASK          (0x0F)
#define FIRE_TYPE_OFF           (0x00)
#define FIRE_TYPE_EMATCH        (0x01)
#define FIRE_TYPE_TALON         (0x02)
#define FIRE_TYPE_METRON        (0x03)

#define FIRE_SUPPLY_MASK        (0xF0)
#define FIRE_SUPPLY_1A          (0x00)
#define FIRE_SUPPLY_3A          (0x10)

#define MAILBOX_SIZE        (128)
static volatile msg_t mailbox_buf[MAILBOX_SIZE];
MAILBOX_DECL(mailbox, mailbox_buf, MAILBOX_SIZE);

static THD_WORKING_AREA(firing_thd_wa, 512);
static THD_FUNCTION(firing_thd, arg) {
    (void)arg;

    msg_t msg;
    msg_t rv;
    m3status_set_ok(M3PYRO_COMPONENT_FIRING);

    while(true) {
        rv = chMBFetch(&mailbox, &msg, TIME_INFINITE);
        if(rv != MSG_OK || msg == 0) {
            m3status_set_error(M3PYRO_COMPONENT_FIRING,
                               M3PYRO_ERROR_FIRE_BAD_MSG);
            continue;
        }

        /* Ignore firing messages while not armed. */
        if(!m3pyro_armed()) {
            continue;
        }

        uint8_t channel = (msg & 0x00FF);
        uint8_t command = (msg & 0xFF00) >> 8;
        uint8_t command_type = command & FIRE_TYPE_MASK;
        uint8_t command_supply = command & FIRE_SUPPLY_MASK;

        if(command_type == FIRE_TYPE_OFF) {
            continue;
        }

        /* Stop continuity measurements while we're firing. */
        m3pyro_continuity_stop();

        /* Enable 1A or 3A supply as appropriate. */
        if(command_supply == FIRE_SUPPLY_1A) {
            m3pyro_1a_enable();
        } else if(command_supply == FIRE_SUPPLY_3A) {
            m3pyro_3a_enable();
        } else {
            m3status_set_error(M3PYRO_COMPONENT_FIRING,
                               M3PYRO_ERROR_FIRE_SUPPLY_UNKNOWN);
            continue;
        }

        /* Confirm the supply has turned on. */
        chThdSleepMilliseconds(2);
        if(m3pyro_read_bus() < 50) {
            m3status_set_error(M3PYRO_COMPONENT_FIRING,
                               M3PYRO_ERROR_FIRE_SUPPLY_FAULT);
            /* Turn both supplies on and hope for the best. */
            m3pyro_1a_enable();
            m3pyro_3a_enable();
        }

        /* Begin firing channel. */
        m3pyro_assert_ch(channel);

        /* Fire for the appropriate duration, or pulse for metrons. */
        if(command_type == FIRE_TYPE_EMATCH) {
            chThdSleepMilliseconds(EMATCH_FIRE_TIME_MS);
        } else if(command_type == FIRE_TYPE_TALON) {
            chThdSleepMilliseconds(TALON_FIRE_TIME_MS);
        } else if(command_type == FIRE_TYPE_METRON) {
            systime_t firing_start = chVTGetSystemTimeX();
            while(ST2MS(chVTTimeElapsedSinceX(firing_start)) < METRON_FIRE_TIME_MS) {
                chThdSleepMilliseconds(10);
                m3pyro_deassert_ch();
                chThdSleepMilliseconds(10);
                m3pyro_assert_ch(channel);
            }
        } else {
            m3status_set_error(M3PYRO_COMPONENT_FIRING,
                               M3PYRO_ERROR_FIRE_TYPE_UNKNOWN);
        }

        /* Cease firing channel. */
        m3pyro_deassert_ch();
        m3pyro_1a_disable();
        m3pyro_3a_disable();

        /* Resume continuity measurements. */
        m3pyro_continuity_restart();
    }
}

/* Start the firing control thread. */
void m3pyro_firing_init(void) {
    m3status_set_init(M3PYRO_COMPONENT_FIRING);

    chThdCreateStatic(firing_thd_wa, sizeof(firing_thd_wa),
                      HIGHPRIO, firing_thd, NULL);
}

/* Enqueue a firing command. */
void m3pyro_firing_enqueue(uint8_t ch, uint8_t command) {
    uint32_t msg = (command<<8) | (ch);
    chMBPost(&mailbox, msg, TIME_IMMEDIATE);
}
