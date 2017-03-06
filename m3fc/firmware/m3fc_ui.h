#ifndef M3FC_UI_H
#define M3FC_UI_H

/* Allow control over how fast the beeper is beeping. */
enum m3fc_ui_beeper_mode {
    M3FC_UI_BEEPER_OFF,
    M3FC_UI_BEEPER_SLOW,
    M3FC_UI_BEEPER_FAST
};
extern enum m3fc_ui_beeper_mode m3fc_ui_beeper_mode;

/* Starts the UI thread which automatically sets the LEDs based on
 * the current m3status */
void m3fc_ui_init(void);

#endif
