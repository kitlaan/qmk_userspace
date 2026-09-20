#include "config.h"
#include "layers.h"
#include "tapdance.h"
#include "keymap_utils.h"

#include QMK_KEYBOARD_H

static tap ql_tap_state = {
    .is_press_action = true,
    .state           = TDS_NOTHING,
};

static int cur_dance(tap_dance_state_t *state) {
    if (state->count == 1) {
        return state->pressed ? TDS_SINGLE_HOLD : TDS_SINGLE_TAP;
    } else if (state->count == 2) {
        return TDS_DOUBLE_TAP;
    } else {
        return TDS_UNKNOWN;
    }
}

static void ql_finished(tap_dance_state_t *state, void *user_data) {
    ql_tap_state.state = cur_dance(state);
    switch (ql_tap_state.state) {
        case TDS_SINGLE_TAP:
            tap_code(KC_MUTE);
            break;
        case TDS_SINGLE_HOLD:
            layer_on(_ADJUST);
            break;
        case TDS_DOUBLE_TAP:
            if (layer_state_is(_ADJUST)) {
                layer_off(_ADJUST);
            } else {
                layer_on(_ADJUST);
            }
            break;
        default:
            break;
    }
}

static void ql_reset(tap_dance_state_t *state, void *user_data) {
    // if the key was held down and now is released then switch off the layer
    if (ql_tap_state.state == TDS_SINGLE_HOLD) {
        layer_off(_ADJUST);
    }
    ql_tap_state.state = TDS_NOTHING;
}

#ifdef FEATURE_DIABLO3

#    define DIABLO_TAP_DANCE(idx) \
        { .fn = {diablo3_tapdance_each, diablo3_tapdance_finish, NULL, NULL}, .user_data = (void *)&(diablo3_spammer[idx]), }

static struct diablo3_timer {
    uint8_t        keycode;
    uint8_t        row;
    uint8_t        col;
    uint32_t       interval;
    deferred_token timer;
} diablo3_spammer[NUM_DIABLO3_TDKEYS] = {
    {.keycode = KC_1, .row = 1, .col = 2},
    {.keycode = KC_2, .row = 2, .col = 2},
    {.keycode = KC_3, .row = 3, .col = 2},
    {.keycode = KC_4, .row = 4, .col = 2},
};

uint32_t diablo3_tap_get(size_t index) {
    return diablo3_spammer[index].interval;
}

static uint32_t diablo3_task(uint32_t trigger_time, void *cb_arg) {
    struct diablo3_timer *d3t = cb_arg;

    if (get_highest_active_layer(layer_state) == _DIABLO3) {
        tap_code(d3t->keycode);
        rgb_matrix_handle_key_event(d3t->row, d3t->col, true);
    } else {
        d3t->timer    = INVALID_DEFERRED_TOKEN;
        d3t->interval = 0;
    }

    return d3t->interval;
}

static void diablo3_tapdance_each(tap_dance_state_t *state, void *user_data) {
    struct diablo3_timer *d3t = user_data;

    if (state->count == 1) {
        tap_code(d3t->keycode);

        // reset the timer just in case
        cancel_deferred_exec(d3t->timer);
        d3t->timer    = INVALID_DEFERRED_TOKEN;
        d3t->interval = 0;
    }
}

static void diablo3_tapdance_finish(tap_dance_state_t *state, void *user_data) {
    struct diablo3_timer *d3t = user_data;

    if (state->count >= 2) {
        const uint8_t diablo_times[] = {1, 2, 3, 5, 8, 13, 21, 34};

        const uint8_t idx = MIN(state->count - 2, ARRAY_SIZE(diablo_times) - 1);
        d3t->interval     = 1000 * diablo_times[idx];
        d3t->timer        = defer_exec(d3t->interval, diablo3_task, d3t);

        // TODO: show the key in a different color to indicate it's active
    }
}
#endif

tap_dance_action_t tap_dance_actions[] = {
    [TD_ENC] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, ql_finished, ql_reset),
    [TD_MAP] = ACTION_TAP_DANCE_DOUBLE(KC_TAB, KC_M),
#ifdef FEATURE_DIABLO3
    [TD_DIABLO3_1] = DIABLO_TAP_DANCE(0),
    [TD_DIABLO3_2] = DIABLO_TAP_DANCE(1),
    [TD_DIABLO3_3] = DIABLO_TAP_DANCE(2),
    [TD_DIABLO3_4] = DIABLO_TAP_DANCE(3),
#endif
};
