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

#pragma once

/* Colour layer names. */
enum {
    RIGHTY_NAV_LAYER_COLOUR = 0,
    FUSION_LAYER_COLOUR,
    CONTROL_LAYER_COLOUR,
    MODE_PICK_LAYER_COLOUR,
    GESTURE_LAYER_COLOUR,
    OPTION_CHANGED_LAYER_COLOUR,
};

/* EEPROM memory for the various persistent states that the A+ can have.
   See eeconfig_init_user() for default values and meanings. */
typedef union {
    uint32_t raw;
    struct {
        bool    horizontal_scroll_arrows : 1;
        bool    vertical_scroll_arrows : 1;
        bool    drag_scroll_mode : 1;
        uint8_t led_brightness : 8;
    };
} user_config_t;

user_config_t user_config;

extern user_config_t user_config;

/* Add custom keycodes for control layer. */
enum my_keycodes {
    PKC_TGL_VERT_SCRL = SAFE_RANGE,
    PKC_TGL_HORIZ_SCRL,
    PKC_TGL_DRAG_SCRL,
    PKC_TGL_ACCEL,
    PKC_GESTURE,
    PKC_MODE,
    PKC_FUSION_ORBIT,
    PKC_FUSION_ROLL,
    PKC_DRAG_SCROLL,
    PKC_ADJUST_LED_BRIGHTNESS,
    PKC_BLINKY_DPI_CONFIG,
};

/* Layer names. Modes take the layers below LAYER_CONTROL and are selected as the
   default layer. Control and the knob-hold layer have to sit above every mode,
   because QMK resolves a key from the highest active layer.

   The layer count is 16 (set in keymap.json) so that these two keep the same
   indices as modes are added. We have plenty of EEPROM, and VIA addresses its
   stored keymap by layer index, so moving them shuffles persistent storage. */
enum {
    LAYER_NAV_RIGHT_HANDED = 0,
    LAYER_FUSION           = 1,
    LAYER_CONTROL          = 14,
    LAYER_KNOB_HOLD        = 15,
};

#define LAYER_MODE_SLOTS LAYER_CONTROL

_Static_assert(LAYER_KNOB_HOLD == DYNAMIC_KEYMAP_LAYER_COUNT - 1, "LAYER_KNOB_HOLD must be the highest layer.");
