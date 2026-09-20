// Copyright 2022 Ted M Lin <tedmlin@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "config.h"

//
// Layer Definitions
//

enum {
    _NUMPAD,
#ifdef FEATURE_FUSION360
    _FUSION360,
#endif
#ifdef FEATURE_DIABLO3
    _DIABLO3,
#endif
#ifdef FEATURE_ELITEDANGEROUS
    _ELITEDANGEROUS,
#endif
    _MACRO,
    _ADJUST,
    NUM_LAYERS
};

#define NUM_LAYERS (_ADJUST + 1)
