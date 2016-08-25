#ifndef M3FC_UI_H
#define M3FC_UI_H

void m3fc_ui_init(void);
void m3fc_ui_led_ok(uint8_t count);
void m3fc_ui_led_warn(uint8_t count);
void m3fc_ui_led_err(uint8_t count);
void m3fc_ui_beep(uint8_t freq);

void m3fc_ui_beep_slow(void);
void m3fc_ui_beep_fast(void);

#endif
