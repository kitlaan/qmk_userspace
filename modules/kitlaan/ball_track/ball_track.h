// Copyright 2026 Ted M Lin
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdint.h>

/* The ball's motion since the last ball_track_reset(), as the modules after
   this one see it. List this module just before the one whose view of the
   motion you want, since a module may zero the report for those after it. */
void    ball_track_reset(void);
int32_t ball_track_x(void);
int32_t ball_track_y(void);
