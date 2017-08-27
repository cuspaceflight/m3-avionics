#ifndef M3PYRO_CONTINUITY
#define M3PYRO_CONTINUITY

/* Initialise the continuity measurements and begin continuous measurements. */
void m3pyro_continuity_init(void);

/* Stop continuity measurements. Disconnects continuity source from bus.
 * This method will block until continuity measurement has ceased.
 */
void m3pyro_continuity_stop(void);

/* Restart continuity measurements. */
void m3pyro_continuity_restart(void);

#endif
