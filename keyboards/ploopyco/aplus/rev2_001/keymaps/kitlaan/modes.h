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

#include "kitlaan.h"

/* Wheel movement for one poll, grouped so that a new field does not change
   every mode's handler signature. A handler may zero a delta it consumes. */
typedef struct {
    int16_t left_delta;
    int16_t right_delta;
    int16_t left_deadzone;
    int16_t right_deadzone;
} wheel_input_t;

/* A mode is a base layer plus the wheel behaviour that belongs with it. The
   buttons come from the layer, but the wheels are read in
   pointing_device_task_user() rather than from the keymap, so each mode needs a
   handler. enter/exit exist so that a mode holding a mouse button down releases
   it on the way out. */
typedef struct {
    uint8_t layer;
    uint8_t colour;
    void (*enter)(void);
    void (*exit)(void);
    void (*tap)(void); /* right knob tapped rather than held */
    void (*wheels)(wheel_input_t* in, report_mouse_t* report);
} mode_t;

/* Mode indices, which are also the picker cells. */
enum {
    MODE_BASE = 0,
    MODE_FUSION,
};

/* A picker cell that no mode claims. */
#define MODE_NONE 0xFF

/* Defined in keymap.c, beside the layers and colours that the table names. */
extern const mode_t  modes[];
extern const uint8_t mode_count;

const mode_t* mode_at(uint8_t mode);
uint8_t       mode_current(void);
void          mode_set(uint8_t mode);

/* The knob-hold state machine. keymap.c's hooks delegate to these, so that the
   flags behind it stay private to modes.c. */
bool mode_process_record(uint16_t keycode, keyrecord_t* record);
void mode_scan(void);
void mode_task(void);

/* gestures.c hands a roll over when the right knob owns the ball. */
bool mode_pick_active(void);
void mode_pick_resolve(uint8_t direction);
