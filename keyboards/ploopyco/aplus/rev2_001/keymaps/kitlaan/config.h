/* Copyright 2020 Christopher Courtney, aka Drashna Jael're  (@drashna) <drashna@live.com>
 * Copyright 2019 Sunjun Kim
 * Copyright 2020 Ploopy Corporation
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

/* Index into ploopyco.c's PLOOPY_DPI_OPTIONS, { 600, 900, 1200, 1600, 2400 }.
   This applies only when the EEPROM is reset; otherwise the stored DPI wins. */
#define PLOOPY_DPI_DEFAULT 2

/* The DPI blink is a count, one per step, so it is slower than the board's
   OPTION_CHANGE_BLINK_TIMEOUT to keep the blinks easy to count. */
#define DPI_BLINK_TIMEOUT 300

/* The keys a Linux host gets for the east and west mouse gestures. These are
   for PaperWM, which stacks workspaces vertically and leaves
   Ctrl+Alt+Left/Right unbound, so the gestures move along its strip of windows
   (switch-right/left). On plain GNOME, these keys tile the window to half the
   screen. Remove both lines to get the Ploopy default, Ctrl+Alt+Right/Left. */
#define GESTURE_LINUX_DESKTOP_NEXT LGUI(KC_RIGHT)
#define GESTURE_LINUX_DESKTOP_PREV LGUI(KC_LEFT)

/* The gestures send different keys to each host OS. Some KVMs keep the board
   powered when they switch hosts, so the board keeps the OS it found first.
   This makes the board reset when the USB starts again, so that it finds the
   new OS. A reset also loses the settings that are not in EEPROM. */
// #define OS_DETECTION_KEYBOARD_RESET

/* Mode picking rolls the ball while the right knob is held. Four directions
   uses the cardinal rolls only and blinks at a diagonal; eight uses them all. */
#define MODE_PICK_DIRECTIONS 4
// #define MODE_PICK_DIRECTIONS 8

/* How long either knob must be held before it stops counting as a tap. The
   left knob still raises LAYER_KNOB_HOLD at once, since only its release can
   tell a tap from a gesture; past this, a release with no roll does nothing. */
#define KNOB_HOLD_TERM 250

/* LED feedback while a knob holds the ball, in ms. A gesture pulses white once
   its roll is long enough to fire. The picker shows mostly purple, with a short
   flash of the colour of the mode that the roll points at. */
#define GESTURE_PULSE_MS 150
#define MODE_PICK_PREVIEW_LONG_MS 600
#define MODE_PICK_PREVIEW_SHORT_MS 200

/* How long Fusion mode holds a modifier before it presses the middle button,
   in ms. See fusion_drag() in keymap.c. */
#define FUSION_MOD_LEAD_MS 20

/* Turn the LEDs off after this long with no input, in ms. Remove it to keep
   them on. */
#define LED_IDLE_TIMEOUT (60 * 60 * 1000UL)
