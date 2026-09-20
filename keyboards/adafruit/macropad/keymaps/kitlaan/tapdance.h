// Copyright 2022 Ted M Lin <tedmlin@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "config.h"

//
// Tap Dance Definitions
//

enum {
    TD_ENC,
    TD_MAP,
#ifdef FEATURE_DIABLO3
    TD_DIABLO3_1,
    TD_DIABLO3_2,
    TD_DIABLO3_3,
    TD_DIABLO3_4,
#endif
};

#define NUM_DIABLO3_TDKEYS 4

//
// Tap States
//

enum {
    TDS_UNKNOWN,
    TDS_NOTHING,
    TDS_SINGLE_TAP,
    TDS_SINGLE_HOLD,
    TDS_DOUBLE_TAP,
};

typedef struct {
    bool is_press_action;
    int  state;
} tap;

#ifdef FEATURE_DIABLO3
uint32_t diablo3_tap_get(size_t index);
#endif
