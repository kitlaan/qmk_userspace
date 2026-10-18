// Copyright 2026 Ted M Lin
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "ball_track.h"

ASSERT_COMMUNITY_MODULES_MIN_API_VERSION(1, 1, 0);

/* Far past any gesture threshold, and far from overflow. */
#define BALL_TRACK_LIMIT 1000000

static int32_t track_x = 0;
static int32_t track_y = 0;

static int32_t clamp_add(int32_t total, int32_t delta) {
    total += delta;
    return total > BALL_TRACK_LIMIT ? BALL_TRACK_LIMIT : total < -BALL_TRACK_LIMIT ? -BALL_TRACK_LIMIT : total;
}

void ball_track_reset(void) {
    track_x = 0;
    track_y = 0;
}

int32_t ball_track_x(void) {
    return track_x;
}

int32_t ball_track_y(void) {
    return track_y;
}

report_mouse_t pointing_device_task_ball_track(report_mouse_t mouse_report) {
    track_x = clamp_add(track_x, mouse_report.x);
    track_y = clamp_add(track_y, mouse_report.y);
    return pointing_device_task_ball_track_kb(mouse_report);
}
