#include "si4460.h"
#include "ch.h"
#include "hal.h"
#include "m3radio_status.h"
#include "m3can.h"

/* XXX Get correct CR1 settings for Si4460 */
static SPIDriver* si4460_spid;
static SPIConfig spi_cfg = {
    .end_cb = NULL,
    .ssport = 0,
    .sspad = 0,
    .cr1 = SPI_CR1_BR_2,
};

static THD_WORKING_AREA(si4460_thd_wa, 512);
static THD_FUNCTION(si4460_thd, arg) {
    (void)arg;

    spiStart(si4460_spid, &spi_cfg);
    m3status_set_ok(M3RADIO_COMPONENT_SI4460);
}

void si4460_init(SPIDriver* spid, ioportid_t ssport, uint32_t sspad)
{
    m3status_set_init(M3RADIO_COMPONENT_SI4460);

    spi_cfg.ssport = ssport;
    spi_cfg.sspad = sspad;
    si4460_spid = spid;

    chThdCreateStatic(si4460_thd_wa, sizeof(si4460_thd_wa), NORMALPRIO,
                      si4460_thd, NULL);
}
