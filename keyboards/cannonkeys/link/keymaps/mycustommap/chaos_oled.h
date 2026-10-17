// chaos_oled.h: QMK glue for chaos.c. Call these from your keymap's hooks so
// the port from your Iris keymap keeps ownership of the user callbacks.
#pragma once

#include QMK_KEYBOARD_H

void chaos_oled_init(void);                                      // keyboard_post_init_user
void chaos_oled_record(uint16_t keycode, keyrecord_t *record);   // process_record_user
void chaos_oled_task(void);                                      // housekeeping_task_user
bool chaos_oled_render(void);                                    // oled_task_user
