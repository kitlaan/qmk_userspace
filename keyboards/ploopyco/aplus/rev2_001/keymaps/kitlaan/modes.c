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

/* For NUM_GESTURE_DIRECTIONS, which the gestures module defines in its own
   introspection header. */
#include "community_modules_introspection.h"
#include "pointing_device_gestures.h"

#include "kitlaan.h"
#include "modes.h"

/* Gesture directions, in the order pointing_device_gestures.h lays them out. */
enum {
    GESTURE_E = 0,
    GESTURE_SE,
    GESTURE_S,
    GESTURE_SW,
    GESTURE_W,
    GESTURE_NW,
    GESTURE_N,
    GESTURE_NE,
};

/* Roll direction -> mode. The cardinals keep their modes whichever count is
   built, so widening to eight does not renumber them. MODE_NONE blinks instead
   of switching, so that a roll onto an unused direction is visible. */
// clang-format off
static const uint8_t mode_pick_cells[NUM_GESTURE_DIRECTIONS] = {
    [GESTURE_N]  = 0,
    [GESTURE_E]  = 1,
    [GESTURE_S]  = 2,
    [GESTURE_W]  = 3,
#if MODE_PICK_DIRECTIONS == 8
    [GESTURE_NE] = 4,
    [GESTURE_SE] = 5,
    [GESTURE_SW] = 6,
    [GESTURE_NW] = 7,
#else
    [GESTURE_NE] = MODE_NONE,
    [GESTURE_SE] = MODE_NONE,
    [GESTURE_SW] = MODE_NONE,
    [GESTURE_NW] = MODE_NONE,
#endif
};
// clang-format on

_Static_assert(MODE_PICK_DIRECTIONS == 4 || MODE_PICK_DIRECTIONS == 8, "MODE_PICK_DIRECTIONS must be 4 or 8.");

static uint8_t current_mode  = MODE_BASE;
static uint8_t previous_mode = MODE_BASE;

/* Knob hold state. Either knob raises LAYER_KNOB_HOLD, so the layer only drops
   once neither of them is holding it up. */
static uint16_t mode_key_timer    = 0;
static bool     mode_key_down     = false;
static bool     mode_key_held     = false; /* past MODE_PICK_HOLD_TERM */
static bool     knob_hold_used    = false; /* the hold has already done something */
static bool     mode_pick_session = false; /* the right knob owns the ball */
static bool     mode_pick_pending = false; /* a pick resolves on the next poll */
static bool     gesture_key_down  = false; /* the left knob holds the layer up */

/* current_mode is always in range, but nothing tells the compiler so, and with a
   single mode defined it deduces the opposite from the mode_set() early return.
   Falling back to the base mode keeps that deduction harmless. */
const mode_t* mode_at(uint8_t mode) {
    return &modes[mode < mode_count ? mode : MODE_BASE];
}

uint8_t mode_current(void) {
    return current_mode;
}

bool mode_pick_active(void) {
    return mode_pick_session;
}

void mode_set(uint8_t mode) {
    /* Refuse a slot with no entry in modes[], which would leave current_mode
       past the end of the table, and refuse a mode that would outrank
       LAYER_CONTROL. The transparent layer itself is harmless:
       layer_switch_get_layer() falls back to layer 0. */
    if (mode >= mode_count || modes[mode].layer >= LAYER_CONTROL) {
        rgblight_blink_layer_repeat(OPTION_CHANGED_LAYER_COLOUR, OPTION_CHANGE_BLINK_TIMEOUT, 3);
        dprintf("No mode %u.\n", mode);
        return;
    }
    if (mode == current_mode) {
        return;
    }

    if (mode_at(current_mode)->exit) {
        mode_at(current_mode)->exit();
    }

    previous_mode = current_mode;
    current_mode  = mode;

    default_layer_set((layer_state_t)1 << modes[mode].layer);

    if (modes[mode].enter) {
        modes[mode].enter();
    }

    dprintf("change mode: %u\n", mode);
}

static void knob_hold_layer_update(void) {
    /* The left knob raises the layer at once, the right knob only once its tap
       window has passed. This tracks the keycode rather than the switch, because
       a mode may put something other than PKC_GESTURE on the left knob, and
       then no release would ever lower the layer again. */
    if (gesture_key_down || mode_key_held) {
        layer_on(LAYER_KNOB_HOLD);
    } else {
        layer_off(LAYER_KNOB_HOLD);
    }
}

void mode_pick_resolve(uint8_t direction) {
    knob_hold_used = true;
    mode_set(direction < NUM_GESTURE_DIRECTIONS ? mode_pick_cells[direction] : MODE_NONE);
}

/* Promote a held right knob out of its tap window. */
void mode_scan(void) {
    if (mode_key_down && !mode_key_held && timer_elapsed(mode_key_timer) > MODE_PICK_HOLD_TERM) {
        mode_key_held  = true;
        knob_hold_used = false;
        knob_hold_layer_update();

        /* One gesture session at a time. If the left knob already owns the ball
           the hold still gives the buttons, but it cannot pick a mode. */
        if (!pointing_device_gestures_is_started()) {
            mode_pick_session = true;
            pointing_device_gestures_start();
            rgblight_set_layer_state(MODE_PICK_LAYER_COLOUR, true);
        }
    }
}

/* Resolve a pick. pointing_device_task_modules() has already run by the time
   wheels.c calls this, so a roll has landed if it is going to. A roll under the
   gesture threshold calls nothing at all, which is what leaves knob_hold_used
   clear and sends us back to the previous mode. */
void mode_task(void) {
    if (mode_pick_pending) {
        mode_pick_pending = false;
        mode_pick_session = false;

        if (!knob_hold_used) {
            mode_set(previous_mode);
        }
    }
}

/* Both knobs. Returns true when the keycode belonged to the knob-hold machine. */
bool mode_process_record(uint16_t keycode, keyrecord_t* record) {
    /* Any press on the knob-hold layer means the hold was used for that, so
       releasing the right knob must not also swap modes. */
    if (record->event.pressed && mode_key_held && keycode != PKC_MODE) {
        knob_hold_used = true;
    }

    switch (keycode) {
        case PKC_GESTURE:
            if (record->event.pressed) {
                gesture_key_down = true;

                /* One gesture session at a time. The right knob may already own
                   the ball for picking, in which case this knob only gives the
                   hold layer. */
                if (!pointing_device_gestures_is_started()) {
                    /* Send correct gesture mode activation based on preference. */
                    pointing_device_gestures_start();
                    rgblight_set_layer_state(GESTURE_LAYER_COLOUR, true);
                }
                knob_hold_layer_update();
            }
            /* Switch is released. */
            else {
                gesture_key_down = false;

                if (!mode_pick_session) {
                    pointing_device_gestures_end();
                    rgblight_set_layer_state(GESTURE_LAYER_COLOUR, false);
                }
                knob_hold_layer_update();
            }
            return true;
        case PKC_MODE:
            if (record->event.pressed) {
                mode_key_timer = timer_read();
                mode_key_down  = true;
            }
            /* Switch is released. */
            else {
                mode_key_down = false;

                if (mode_key_held) {
                    mode_key_held = false;
                    knob_hold_layer_update();
                    rgblight_set_layer_state(MODE_PICK_LAYER_COLOUR, false);

                    if (mode_pick_session) {
                        /* The roll direction lands one poll later, once
                           pointing_device_task_modules() has run. */
                        pointing_device_gestures_end();
                        mode_pick_pending = true;
                    }
                }
                /* Released inside the tap window. */
                else if (mode_at(current_mode)->tap) {
                    mode_at(current_mode)->tap();
                }
            }
        default:
            return false;
    }
}
