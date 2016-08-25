#ifndef M3FC_UI_H
#define M3FC_UI_H

/* Starts the UI thread which automatically sets the LEDs based on
 * the current m3status */
void m3fc_ui_init(void);

/* Sets whether the beeper should indicate "armed" or "disarmed" */
void m3fc_ui_set_armed(bool armed);

#endif
