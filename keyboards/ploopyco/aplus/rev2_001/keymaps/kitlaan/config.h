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

/* Mode picking rolls the ball while the right knob is held. Four directions
   uses the cardinal rolls only and blinks at a diagonal; eight uses them all. */
#define MODE_PICK_DIRECTIONS 4
//#define MODE_PICK_DIRECTIONS 8

/* How long the right knob must be held before it stops counting as a tap. The
   left knob has no tap action, so it raises LAYER_KNOB_HOLD at once. */
#define MODE_PICK_HOLD_TERM 250
