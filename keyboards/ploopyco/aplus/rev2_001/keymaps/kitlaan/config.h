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

/* Mode picking rolls the ball while the right knob is held. Four directions
   uses the cardinal rolls only and blinks at a diagonal; eight uses them all. */
#define MODE_PICK_DIRECTIONS 4
//#define MODE_PICK_DIRECTIONS 8

/* How long the right knob must be held before it stops counting as a tap. The
   left knob has no tap action, so it raises LAYER_KNOB_HOLD at once. */
#define MODE_PICK_HOLD_TERM 250
