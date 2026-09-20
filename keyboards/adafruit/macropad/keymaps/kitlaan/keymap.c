/* Copyright 2022 Ted M Lin <tedmlin@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include QMK_KEYBOARD_H

#include "config.h"
#include "layers.h"
#include "tapdance.h"
#include "keymap_utils.h"

#define TD_D3(x) TD(TD_DIABLO3_##x)

// clang-format off
const uint16_t PROGMEM rotL_indexes[] = {
    0, 3, 6, 9, 12,
       2, 5, 8, 11,
       1, 4, 7, 10
};
// clang-format on

enum custom_keycodes {
    M_ED_SEL1 = SAFE_RANGE,
    M_ED_SEL1S,
    M_ED_CLEAR,
};

// clang-format off
const uint16_t PROGMEM keymaps[NUM_LAYERS][MATRIX_ROWS][MATRIX_COLS] = {
    [_NUMPAD] = LAYOUT(
                                TD(TD_ENC),
        KC_ENT,     KC_0,       KC_BSPC,
        KC_7,       KC_8,       KC_9,
        KC_4,       KC_5,       KC_6,
        KC_1,       KC_2,       KC_3
    ),
#ifdef FEATURE_DIABLO3
    [_DIABLO3] = LAYOUT_rotL(
        TD(TD_ENC),    TD_D3(1),      TD_D3(2),      TD_D3(3),      TD_D3(4),
                       KC_S,          KC_I,          KC_T,          KC_Q,
                       ALT_T(KC_ENT), LSFT(KC_J),    TD(TD_MAP),    SFT_T(KC_SPACE)
    ),
#endif
#ifdef FEATURE_ELITEDANGEROUS
    [_ELITEDANGEROUS] = LAYOUT_rotL(
        //             silent         heat sink      shield cell    chaff
        TD(TD_ENC),    KC_DEL,        KC_V,          KC_B,          KC_C,
        //             landing        cargo          hardpoint      supercruise
                       KC_L,          KC_HOME,       KC_U,          RSFT(KC_J),
        //             ecm            tharg field    ...            nightvis
                       KC_COMMA,      KC_K,          XXXXXXX,       RSFT(KC_N)
    ),
#endif
    [_MACRO] = LAYOUT_rotL(
        //             n/a            n/a            n/a            n/a
        TD(TD_ENC),    XXXXXXX,       XXXXXXX,       XXXXXXX,       XXXXXXX,
        //             n/a            n/a            n/a            n/a
                       XXXXXXX,       XXXXXXX,       XXXXXXX,       XXXXXXX,
        //             n/a            n/a            n/a            n/a
                       M_ED_SEL1,     M_ED_SEL1S,    M_ED_CLEAR,    XXXXXXX
    ),
    [_ADJUST] = LAYOUT(
                                TG(_ADJUST),
        _______,    _______,    _______,
        _______,    _______,    _______,
        _______,    _______,    _______,
        _______,    _______,    _______
    ),
};
// clang-format on

void set_layer_color(int layer) {
    switch (layer) {
        case _NUMPAD:
            rgb_matrix_mode_noeeprom(RGB_MATRIX_SOLID_COLOR);
            rgb_matrix_sethsv_noeeprom(0, 0, 30);
            break;
#ifdef FEATURE_DIABLO3
        case _DIABLO3:
            rgb_matrix_mode_noeeprom(RGB_MATRIX_SOLID_REACTIVE);
            rgb_matrix_sethsv_noeeprom(250, 230, 80);
            break;
#endif
#ifdef FEATURE_ELITEDANGEROUS
        case _ELITEDANGEROUS:
            rgb_matrix_mode_noeeprom(RGB_MATRIX_SOLID_REACTIVE);
            rgb_matrix_sethsv_noeeprom(80, 230, 80);
            break;
#endif
        case _ADJUST:
            rgb_matrix_mode_noeeprom(RGB_MATRIX_SOLID_COLOR);
            rgb_matrix_sethsv_noeeprom(148, 142, 80);
            break;
        default:
            break;
    }
}

layer_state_t layer_state_set_user(layer_state_t state) {
    set_layer_color(get_highest_active_layer(state));
    return state;
}

void keyboard_post_init_user(void) {
    set_layer_color(_NUMPAD);
}

#define SS_TAPDD(x, n) SS_DOWN(x) SS_DELAY(n) SS_UP(x) SS_DELAY(250)

struct macro_entry ed_sell[] = {
    {KC_SPACE, 100, 400}, {KC_A, 50, 200}, {KC_W, 50, 200}, {KC_W, 50, 200}, {KC_A, 2500, 200}, {KC_D, 50, 200}, {KC_S, 50, 400}, {KC_SPACE, 50, 3000},
};
deferred_token macro_ed_repeater = INVALID_DEFERRED_TOKEN;

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case M_ED_SEL1:
            if (record->event.pressed) {
                SEND_STRING(SS_TAPDD(X_SPACE, 50) SS_TAPDD(X_A, 50) SS_TAPDD(X_W, 50) SS_TAPDD(X_A, 2500) SS_TAPDD(X_D, 50) SS_TAPDD(X_S, 50));
            }
            break;
        case M_ED_SEL1S:
            if (record->event.pressed) {
                if (macro_ed_repeater == INVALID_DEFERRED_TOKEN) {
                    macro_ed_repeater = macro_start(ed_sell, ARRAY_SIZE(ed_sell));
                } else {
                    macro_ed_repeater = macro_cancel(macro_ed_repeater);
                }
            }
            break;
        case M_ED_CLEAR:
            if (record->event.pressed) {
                clear_keyboard();
            }
            break;
    }
    return true;
};

#if 0
// this overrides EVERYTHING, including "reactive"
// INSTEAD, we need to set an override of the default "solid" value...

// TODO: maybe we detect if the LED is "not" a specific baseline value, and only change
// when that's the case

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    switch (get_highest_active_layer(layer_state)) {
#    ifdef FEATURE_DIABLO3
        case _DIABLO3:
            for (size_t idx = 0; idx < NUM_DIABLO3_TDKEYS; idx++) {
                if (diablo3_tap_get(idx)) {
                    const uint16_t D3_INDEXES[NUM_DIABLO3_TDKEYS] = {2, 5, 8, 11};
                    RGB_MATRIX_INDICATOR_SET_COLOR(D3_INDEXES[idx], 0, 50, 0);
                }
            }
            break;
#    endif
        default:
            break;
    }

    return false;
}
#endif
