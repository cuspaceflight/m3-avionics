#ifndef M3PYRO_HAL
#define M3PYRO_HAL

/* Initialise the HAL. Sets up ADCs etc. */
void m3pyro_hal_init(void);

/* Read bus voltage. Returns in units of 0.1V, e.g. 33 for 3.3V. */
uint8_t m3pyro_read_bus(void);
/* Read supply voltage. Returns in units of 0.1V, e.g. 74 for 7.4V. */
uint8_t m3pyro_read_supply(void);
/* Read continuity. Returns a calculated resistance in 2ohms, or 255 for hi. */
uint8_t m3pyro_read_cont(void);

/* Enable the continuity measurement current. */
void m3pyro_cont_enable(void);
/* Disable the continuity measurement current, setting the pin hi-z. */
void m3pyro_cont_disable(void);
/* Use the continuity measurement to discharge the bus. */
void m3pyro_cont_gnd(void);

/* Assert a channel, connecting it to the bus. */
void m3pyro_assert_ch(uint8_t channel);
/* De-assert all channels, disconnecting them from the bus. */
void m3pyro_deassert_ch(void);

/* Enable the 1A constant current supply, energizing the bus. */
void m3pyro_1a_enable(void);
/* Disable the 1A constant current supply. */
void m3pyro_1a_disable(void);
/* Enable the 3A constant current supply, energizing the bus. */
void m3pyro_3a_enable(void);
/* Disable the 3A constant current supply. */
void m3pyro_3a_disable(void);

#endif
