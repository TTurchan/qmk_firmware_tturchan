// Copyright 2025 TTurchan
// SPDX-License-Identifier: GPL-2.0-or-later
#include QMK_KEYBOARD_H
#include "chaos_oled.h"

#define _LAYER0 0
#define _LAYER1 1
#define _LAYER2 2
#define _LAYER3 3

enum custom_keycodes {
    LAYER0 = SAFE_RANGE,
    LAYER1,
    LAYER2,
    LAYER3,
};

#ifdef COMBO_ENABLE
const uint16_t PROGMEM paren_combo1[] = {KC_D, KC_F, COMBO_END};
const uint16_t PROGMEM paren_combo2[] = {KC_J, KC_K, COMBO_END};
const uint16_t PROGMEM brace_combo1[] = {KC_S, KC_F, COMBO_END};
const uint16_t PROGMEM brace_combo2[] = {KC_L, KC_J, COMBO_END};
const uint16_t PROGMEM equal_combo[]  = {KC_X, KC_C, COMBO_END};
const uint16_t PROGMEM minus_combo[]  = {KC_V, KC_C, COMBO_END};
const uint16_t PROGMEM ctrlb_combo[]  = {KC_I, KC_J, COMBO_END};
const uint16_t PROGMEM ctrla_combo[]  = {KC_K, KC_M, COMBO_END};
const uint16_t PROGMEM ampr_combo[]   = {KC_M, KC_N, COMBO_END};
combo_t key_combos[] = {
    COMBO(paren_combo1, KC_LPRN),
    COMBO(paren_combo2, KC_RPRN),
    COMBO(brace_combo1, KC_LBRC),
    COMBO(brace_combo2, KC_RBRC),
    COMBO(equal_combo,  KC_EQUAL),
    COMBO(minus_combo,  KC_MINUS),
    COMBO(ctrlb_combo,  LCTL(KC_B)),
    COMBO(ctrla_combo,  LCTL(KC_A)),
    COMBO(ampr_combo,   KC_AMPR),
};
#endif

// Link LAYOUT: 12 + 12 + 12 + 14 + 10 = 60 keys
// Row 0  (12): ESC  1    2    3    4    5    |  6    7    8    9    0    DEL
// Row 1  (12): Tab  Q    W    E    R    T    |  Y    U    I    O    P    Ent
// Row 2  (12): Ctl  A    S    D    F    G    |  H    J    K    L    ;    '
// Row 3  (14): Sft  Z    X    C    V    B  Mute|Play  N    M    ,    .    /    Bsls
// Thumb  (10): MO2 GUI  Alt  MO1  Spc  | Sft Bspc RAlt  __   __

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

[_LAYER0] = LAYOUT(
    QK_GESC, KC_1,    KC_2,    KC_3,    KC_4,    KC_5,                       KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_DEL,
    KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,                       KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_ENT,
    KC_LCTL, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,                       KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT,
    KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_MUTE, KC_MPLY,  KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_BSLS,
             MO(2),   KC_LGUI, KC_LALT, MO(1),   KC_SPC,                    KC_LSFT, KC_BSPC, KC_RALT, KC_TRNS, KC_TRNS
),

[_LAYER1] = LAYOUT(
    KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,                      KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,
    KC_GRV,  KC_MPRV, KC_MPLY, KC_MNXT, KC_TRNS, KC_TRNS,                    KC_HOME, KC_PGDN, KC_PGUP, KC_END,  KC_TRNS, KC_TRNS,
    KC_TRNS, KC_TRNS, KC_VOLD, KC_VOLU, KC_MUTE, KC_TRNS,                    KC_LEFT, KC_DOWN, KC_UP,   KC_RGHT, KC_TRNS, KC_TRNS,
    KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
             KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,                    KC_TRNS, KC_DEL,  KC_TRNS, KC_TRNS, KC_TRNS
),

[_LAYER2] = LAYOUT(
    KC_F12,  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,                      KC_F6,   KC_F7,   QK_BOOT, QK_RBT,  DB_TOGG, EE_CLR,
    KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,                    KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
    KC_TRNS, KC_TRNS, KC_VOLD, KC_VOLU, KC_MUTE, KC_TRNS,                    KC_HOME, KC_PGDN, KC_PGUP, KC_END,  KC_TRNS, KC_TRNS,
    KC_TRNS, KC_TRNS, KC_MPRV, KC_MNXT, KC_MPLY, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
             KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,                    KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
),

[_LAYER3] = LAYOUT(
    KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,                      KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,
    KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,                      KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,
    KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,                      KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,
    KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,    KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,
             KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,                      KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO
),

};

void keyboard_post_init_user(void) {
    chaos_oled_init();
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    chaos_oled_record(keycode, record);
    return true;
}

void housekeeping_task_user(void) {
    chaos_oled_task();
}

#ifdef OLED_ENABLE
oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    return OLED_ROTATION_0;
}

bool oled_task_user(void) {
    return chaos_oled_render();
}
#endif
