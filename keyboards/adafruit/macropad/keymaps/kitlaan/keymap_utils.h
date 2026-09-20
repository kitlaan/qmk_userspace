// Copyright 2022 Ted M Lin <tedmlin@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Helper to define layout as if the macropad is rotated right
// which puts the encoder to the right (and OLED to the left).
// clang-format off
#define LAYOUT_rotR(A, B, C, D,    \
                    E, F, G, H,    \
                    I, J, K, L, R) \
    LAYOUT(      R, \
           D, H, L, \
           C, G, K, \
           B, F, J, \
           A, E, I)
// clang-format on

// Helper to define layout as if the macropad is rotated left
// which puts the encoder to the left (and OLED to the right).
// clang-format off
#define LAYOUT_rotL(R, A, B, C, D,    \
                       E, F, G, H,    \
                       I, J, K, L) \
    LAYOUT(      R, \
           I, E, A, \
           J, F, B, \
           K, G, C, \
           L, H, D)
// clang-format on

int get_default_layer(void);
int get_highest_active_layer(int state);

//
struct macro_entry {
    uint16_t code;
    uint32_t hold;
    uint32_t delay;
};

uint8_t macro_start(struct macro_entry *codes, size_t codelen);
uint8_t macro_cancel(uint8_t token);
bool    macro_is_active(void);
