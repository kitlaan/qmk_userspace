#include "config.h"
#include "layers.h"
#include "keymap_utils.h"
#include "tapdance.h"

#include <stdio.h>

#include QMK_KEYBOARD_H

static const char* PROGMEM layer_names[NUM_LAYERS] = {
    [_NUMPAD] = "numpad",
#ifdef FEATURE_FUSION360
    [_FUSION360] = "fusion 360",
#endif
#ifdef FEATURE_DIABLO3
    [_DIABLO3] = "diablo 3",
#endif
#ifdef FEATURE_ELITEDANGEROUS
    [_ELITEDANGEROUS] = "elite dangerous",
#endif
    [_MACRO]  = "macro",
    [_ADJUST] = "adjust",
};

#ifdef FEATURE_DIABLO3
void oled_diablo3(void) {
    oled_write_P(PSTR("\n"), false);
    for (size_t idx = 0; idx < NUM_DIABLO3_TDKEYS; idx++) {
        char buffer[12];
        snprintf(buffer, sizeof(buffer), "%2lu ", diablo3_tap_get(idx) / 1000);
        oled_write(buffer, false);
    }
}
#endif

bool oled_task_user(void) {
    oled_write_P(layer_state_is(_ADJUST) ? PSTR("[") : PSTR(" "), false);
    oled_write_P(layer_names[get_highest_layer(default_layer_state)], false);
    oled_write_P(layer_state_is(_ADJUST) ? PSTR("]\n") : PSTR(" \n"), false);

#ifdef FEATURE_DIABLO3
    if (get_highest_active_layer(layer_state) == _DIABLO3) {
        oled_diablo3();
    }
#endif

    if (get_highest_active_layer(layer_state) == _MACRO) {
        oled_write_P(macro_is_active() ? PSTR("running\n") : PSTR("       \n"), false);
    }

    return true;
}
