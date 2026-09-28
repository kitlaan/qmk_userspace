/* Copyright 2023 Colin Lam (Ploopy Corporation)
 * Copyright 2020 Christopher Courtney, aka Drashna Jael're  (@drashna) <drashna@live.com>
 * Copyright 2019 Sunjun Kim
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

#include "rgblight.h"
#include "printf.h"

#include "pointing_device_accel.h"
#include "pointing_device_gestures.h"

#include "kitlaan.h"
#include "wheels.h"

/* Layer lighting, used to blink when making gestures. */
// clang-format off
const rgblight_segment_t PROGMEM righty_nav_layer_colour[] =        RGBLIGHT_LAYER_SEGMENTS( {0, 2, HSV_NAVBLUE} );
const rgblight_segment_t PROGMEM control_layer_colour[] =           RGBLIGHT_LAYER_SEGMENTS( {0, 2, HSV_RED} );
const rgblight_segment_t PROGMEM gesture_layer_colour[] =           RGBLIGHT_LAYER_SEGMENTS( {0, 2, HSV_GESTUREYELLOW} );
const rgblight_segment_t PROGMEM option_changed_layer_colour[] =    RGBLIGHT_LAYER_SEGMENTS( {0, 2, HSV_OPTIONCHANGED} );

/* Lighting layer definitions. Later layers take precedence, so these
   are defined in the order that they will be displayed in. */
const rgblight_segment_t* const PROGMEM my_rgb_layers[] = RGBLIGHT_LAYERS_LIST(
    righty_nav_layer_colour,
    control_layer_colour,
    gesture_layer_colour,
    option_changed_layer_colour
);
// clang-format on

user_config_t user_config;

/* Mouse gestures keymap. This has to stay in the keymap translation unit: the
   gestures module reads the array from its own introspection.c with no extern
   declaration, so it only resolves where the keymap is compiled. The actions
   themselves live in gestures.c. */
// clang-format off
const uint16_t PROGMEM pointing_device_gestures[NUM_GESTURE_DIRECTIONS] =
    GESTURES_CARDINAL_AND_ORDINAL_DIRECTIONS( KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO );
// clang-format on

/* Keymap. */
// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    // Base layer with all of the mouse-related stuff for everyday use.
    [LAYER_NAV_RIGHT_HANDED] = LAYOUT(  MS_BTN4, MS_BTN5, PKC_DRAG_SCROLL, MS_BTN2,
                                        MS_BTN1, MS_BTN3,
                                        PKC_GESTURE, TG(LAYER_CONTROL) ),
    // Layer for all of the customization options.
    [LAYER_CONTROL] = LAYOUT(           PKC_BLINKY_DPI_CONFIG, PKC_ADJUST_LED_BRIGHTNESS, PKC_TGL_VERT_SCRL, PKC_TGL_HORIZ_SCRL,
                                        PKC_TGL_ACCEL, PKC_TGL_DRAG_SCRL,
                                        TG(LAYER_CONTROL), TG(LAYER_CONTROL) ),
    [2] = LAYOUT( _______, _______, _______, _______, _______, _______, _______, _______ ),
    [3] = LAYOUT( _______, _______, _______, _______, _______, _______, _______, _______ ),
    [4] = LAYOUT( _______, _______, _______, _______, _______, _______, _______, _______ ),
    [5] = LAYOUT( _______, _______, _______, _______, _______, _______, _______, _______ ),
    [6] = LAYOUT( _______, _______, _______, _______, _______, _______, _______, _______ ),
    [7] = LAYOUT( _______, _______, _______, _______, _______, _______, _______, _______ )
};
// clang-format on

/* Called whenever layers are modified. */
layer_state_t layer_state_set_user(layer_state_t state) {
    /* Set layer colours. */
    switch (get_highest_layer(state)) {
        case LAYER_NAV_RIGHT_HANDED:
            rgblight_set_layer_state(RIGHTY_NAV_LAYER_COLOUR, true);

            rgblight_set_layer_state(CONTROL_LAYER_COLOUR, false);

            dprintf("change layer: nav\n");
            break;
        case LAYER_CONTROL:
            rgblight_set_layer_state(CONTROL_LAYER_COLOUR, true);

            rgblight_set_layer_state(RIGHTY_NAV_LAYER_COLOUR, false);

            dprintf("change layer: control\n");
            break;
        default:
            rgblight_setrgb(RGB_CYAN);
            dprintf("change layer: oopsie, invalid layer!\n");
            break;
    }
    return state;
}

/* Set sane defaults when EEPROM is reset. */
void eeconfig_init_user(void) {
    user_config.raw = 0;

    user_config.horizontal_scroll_arrows = false; // False = Scroll events, True = left/right arrows.
    user_config.vertical_scroll_arrows   = false; // False = Scroll events, True = up/down arrows.
    user_config.drag_scroll_mode         = false; // Hold to activate.
    user_config.led_brightness           = 4;     // Brightest mode by default.

    eeconfig_update_user(user_config.raw);
}

void keyboard_pre_init_user(void) {
    wheels_pre_init();
}

void keyboard_post_init_user(void) {
    /* ============================ Debug Setup ============================ */

    /* If you turn these on, also uncomment CONSOLE_ENABLE = yes in
       post_rules.mk! */
    // debug_enable = true;
    // debug_matrix=true;
    // debug_keyboard=true;
    // debug_mouse=true;

    /* ========================== Scroll Wheel Setup ======================= */
    wheels_init();

    /* =================== Persistent Configuration Setup ================== */

    /* Load up configuration details from the EEPROM. */
    user_config.raw = eeconfig_read_user();

    /* ============================ Lighting Setup ========================= */

    /* Enable the LED layers. */
    rgblight_layers = my_rgb_layers;

    /* Enable RGB lighting for layer indicators. */
    rgblight_enable_noeeprom();
    rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT);

    /* Set an initial colour to establish a baseline brightness for the RGB layers. */
    rgblight_sethsv(0, 0, RGBLIGHT_VAL_STEP * user_config.led_brightness);
}

bool process_record_user(uint16_t keycode, keyrecord_t* record) {
    switch (keycode) {
        case PKC_TGL_VERT_SCRL:
            if (record->event.pressed) {
                user_config.vertical_scroll_arrows ^= 1;
                eeconfig_update_user(user_config.raw);

                if (user_config.vertical_scroll_arrows) {
                    rgblight_blink_layer(OPTION_CHANGED_LAYER_COLOUR, OPTION_CHANGE_BLINK_TIMEOUT * 2);
                    dprintf("Vert. scroll = arrows.\n");
                } else {
                    rgblight_blink_layer_repeat(OPTION_CHANGED_LAYER_COLOUR, OPTION_CHANGE_BLINK_TIMEOUT, 2);
                    dprintf("Vert. scroll = scroll events.\n");
                }
            }
            return true;
        case PKC_TGL_HORIZ_SCRL:
            if (record->event.pressed) {
                user_config.horizontal_scroll_arrows ^= 1;
                eeconfig_update_user(user_config.raw);

                if (user_config.horizontal_scroll_arrows) {
                    rgblight_blink_layer(OPTION_CHANGED_LAYER_COLOUR, OPTION_CHANGE_BLINK_TIMEOUT * 2);
                    dprintf("Horiz. scroll = arrows.\n");
                } else {
                    rgblight_blink_layer_repeat(OPTION_CHANGED_LAYER_COLOUR, OPTION_CHANGE_BLINK_TIMEOUT, 2);
                    dprintf("Horiz. scroll = scroll events.\n");
                }
            }
            return true;
        case PKC_TGL_DRAG_SCRL:
            if (record->event.pressed) {
                user_config.drag_scroll_mode ^= 1;
                eeconfig_update_user(user_config.raw);

                if (user_config.drag_scroll_mode) {
                    rgblight_blink_layer(OPTION_CHANGED_LAYER_COLOUR, OPTION_CHANGE_BLINK_TIMEOUT * 2);
                    dprintf("Drag scroll: toggle to activate.\n");
                } else {
                    rgblight_blink_layer_repeat(OPTION_CHANGED_LAYER_COLOUR, OPTION_CHANGE_BLINK_TIMEOUT, 2);
                    dprintf("Drag scroll: hold to activate.\n");
                }
            }
            return true;
        case PKC_TGL_ACCEL:
            if (record->event.pressed) {
                /* Not MA_TOGG, so that we can blink. */
                pointing_device_accel_toggle_enabled();

                if (pointing_device_accel_get_enabled()) {
                    rgblight_blink_layer(OPTION_CHANGED_LAYER_COLOUR, OPTION_CHANGE_BLINK_TIMEOUT * 2);
                    dprintf("Accel on.\n");
                } else {
                    rgblight_blink_layer_repeat(OPTION_CHANGED_LAYER_COLOUR, OPTION_CHANGE_BLINK_TIMEOUT, 2);
                    dprintf("Accel off.\n");
                }
            }
            return true;
        case PKC_GESTURE:
            if (record->event.pressed) {
                /* Send correct gesture mode activation based on preference. */
                pointing_device_gestures_start();
                rgblight_set_layer_state(GESTURE_LAYER_COLOUR, true);
            }
            /* Switch is released. */
            else {
                pointing_device_gestures_end();
                rgblight_set_layer_state(GESTURE_LAYER_COLOUR, false);
            }
            return true;
        case PKC_DRAG_SCROLL:
            if (user_config.drag_scroll_mode) {
                if (record->event.pressed) {
                    toggle_drag_scroll();
                }
            }
            /* Press and hold mode. */
            else {
                is_drag_scroll = record->event.pressed;
            }
            return true;
        case PKC_ADJUST_LED_BRIGHTNESS:
            /* Increase brightness in RGBLIGHT_VAL_STEP steps.
               Go to minimum brightness if the max brightness is hit. */
            if (record->event.pressed) {
                if (rgblight_get_val() >= RGBLIGHT_VAL_STEP * 4) {
                    rgblight_decrease_val_noeeprom();
                    rgblight_decrease_val_noeeprom();
                    rgblight_decrease_val_noeeprom();
                    user_config.led_brightness = 1;
                    eeconfig_update_user(user_config.raw);
                } else {
                    rgblight_increase_val_noeeprom();
                    user_config.led_brightness += 1;
                    eeconfig_update_user(user_config.raw);
                }

                dprintf("Adjusted LED brightness. Current brightness = %d/255\n", rgblight_get_val());
            }
            return true;
        case PKC_BLINKY_DPI_CONFIG:
            if (record->event.pressed) {
                cycle_dpi();

                dprintf("DPI changed. Current DPI = %d\n", dpi_array[keyboard_config.dpi_config]);
                rgblight_blink_layer_repeat(OPTION_CHANGED_LAYER_COLOUR, OPTION_CHANGE_BLINK_TIMEOUT, keyboard_config.dpi_config + 1);
            }
            return true;
        default:
            return true;
    }
}
