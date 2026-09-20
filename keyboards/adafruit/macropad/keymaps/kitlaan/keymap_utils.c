#include "config.h"
#include "layers.h"
#include "keymap_utils.h"

#include QMK_KEYBOARD_H

int get_default_layer(void) {
    for (int ix = 0; ix < _ADJUST; ++ix) {
        if (IS_LAYER_ON_STATE(default_layer_state, ix)) {
            return ix;
        }
    }
    return _NUMPAD;
}

int get_highest_active_layer(int state) {
    return state ? get_highest_layer(state) : get_highest_layer(default_layer_state);
}

static size_t              macro_ix      = 0;
static size_t              macro_len     = 0;
static struct macro_entry *macro_codes   = NULL;
static uint16_t            macro_lastkey = KC_NO;
static uint32_t            macro_delay   = 0;

static uint32_t macro_exec_task(uint32_t trigger_time, void *cb_arg) {
    if (macro_lastkey != KC_NO) {
        if (macro_lastkey != KC_TRANSPARENT) {
            unregister_code(macro_lastkey);
        }
        macro_lastkey = KC_NO;
        macro_ix      = (macro_ix + 1) % macro_len;
        return macro_delay;
    } else {
        if (macro_codes[macro_ix].code != KC_TRANSPARENT) {
            register_code(macro_codes[macro_ix].code);
        }
        macro_lastkey = macro_codes[macro_ix].code;
        macro_delay   = macro_codes[macro_ix].delay;
        return macro_codes[macro_ix].hold;
    }
}

uint8_t macro_start(struct macro_entry *codes, size_t codelen) {
    macro_delay   = 0;
    macro_codes   = codes;
    macro_len     = codelen;
    macro_ix      = 0;
    macro_lastkey = KC_NO;
    return defer_exec(10, macro_exec_task, NULL);
}

uint8_t macro_cancel(deferred_token token) {
    if (macro_lastkey != KC_NO) {
        if (macro_lastkey != KC_TRANSPARENT) {
            unregister_code(macro_lastkey);
        }
    }
    cancel_deferred_exec(token);
    macro_codes = NULL;
    return INVALID_DEFERRED_TOKEN;
}

bool macro_is_active() {
    return macro_codes != NULL;
}
