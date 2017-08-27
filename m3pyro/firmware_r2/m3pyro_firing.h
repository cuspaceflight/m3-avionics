#ifndef M3PYRO_FIRING
#define M3PYRO_FIRING

/* Start the firing control thread. */
void m3pyro_firing_init(void);

/* Enqueue a firing command. */
void m3pyro_firing_enqueue(uint8_t ch, uint8_t command);

#endif
