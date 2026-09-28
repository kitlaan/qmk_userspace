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

#include "pointing_device_gestures.h"

#include "kitlaan.h"
#include "modes.h"

/* The mouse gesture actions from the Ploopy default keymap. */
static void mouse_gesture(uint8_t direction) {
    /* Directions are numbered off 0-7, starting with east. */
    switch (direction) {
        case 0: /* East, Next Virtual Desktop. */
            /* On Windows, send CTRL + LGUI + RIGHT. */
            if (detected_host_os() == OS_WINDOWS) {
                tap_code16(LCG(KC_RIGHT));
            }
            /* On Linux, send CTRL + ALT + RIGHT. */
            else if (detected_host_os() == OS_LINUX) {
                tap_code16(LCA(KC_RIGHT));
            }
            /* On MacOs, send Control + RIGHT. */
            else {
                tap_code16(LCTL(KC_RIGHT));
            }
            break;
        case 1: /* South-East, Paste. */
            tap_code16(C(KC_V));
            break;
        case 2: /* South, Copy. */
            tap_code16(C(KC_C));
            break;
        case 3: /* South-West, Cut. */
            tap_code16(C(KC_X));
            break;
        case 4: /* West, Previous Virtual Desktop. */
            /* On Windows, send CTRL + LGUI + LEFT. */
            if (detected_host_os() == OS_WINDOWS) {
                tap_code16(LCG(KC_LEFT));
            }
            /* On Linux, send CTRL + ALT + LEFT. */
            else if (detected_host_os() == OS_LINUX) {
                tap_code16(LCA(KC_LEFT));
            }
            /* On MacOs, send Control + LEFT. */
            else {
                tap_code16(LCTL(KC_LEFT));
            }
            break;
        case 5: /* North-West, Undo. */
            tap_code16(C(KC_Z));
            break;
        case 6: /* North, Play/Pause Audio. */
            tap_code16(KC_MEDIA_PLAY_PAUSE);
            break;
        case 7: /* North-East, Redo. */
            /* On Windows/Linux, send CTRL + Y. */
            if (detected_host_os() == OS_WINDOWS || detected_host_os() == OS_LINUX) {
                tap_code16(C(KC_Y));
            }
            /* On MacOs, send CMD + SHIFT + Z. */
            else {
                register_code(KC_LCMD);
                register_code(KC_LSFT);
                tap_code(KC_Z);
                unregister_code(KC_LCMD);
                unregister_code(KC_LSFT);
            }
            break;
        default:
            break;
    }

    /* Flash light to indicate successful gesture processing. */
    rgblight_blink_layer(OPTION_CHANGED_LAYER_COLOUR, OPTION_CHANGE_BLINK_TIMEOUT * 2);
}

/* Override default function in submodule code so that we can blink whenever
   a successful gesture is registered. */
void pointing_device_gestures_trigger(uint8_t direction) {
    /* Both knobs roll the ball through this one override, so it branches on
       whichever of them opened the session. */
    if (mode_pick_active()) {
        mode_pick_resolve(direction);
        return;
    }

    mouse_gesture(direction);
}
