#include "config.h"
#include "layers.h"
#include "keymap_utils.h"

#include QMK_KEYBOARD_H

bool encoder_update_user(uint8_t index, bool clockwise) {
    switch (get_highest_active_layer(layer_state)) {
        case _ADJUST: {
            int layer = get_default_layer();
            if (clockwise) {
                layer = (layer + 1) % _ADJUST;
            } else {
                if (layer == 0) {
                    layer = _ADJUST;
                }
                layer = (layer - 1) % _ADJUST;
            }
            default_layer_set(1UL << layer);
            return false;
        }
    }
    return true;
}
