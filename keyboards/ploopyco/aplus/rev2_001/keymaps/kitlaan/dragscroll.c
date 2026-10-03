/* Copyright 2020 Christopher Courtney, aka Drashna Jael're  (@drashna) <drashna@live.com>
 * Copyright 2019 Sunjun Kim
 * Copyright 2020 Ploopy Corporation
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

#include "dragscroll.h"

/* The board's config.h sets these. The fallbacks match ploopyco.c. */
#ifndef PLOOPY_DRAGSCROLL_DIVISOR_H
#    define PLOOPY_DRAGSCROLL_DIVISOR_H 8.0
#endif
#ifndef PLOOPY_DRAGSCROLL_DIVISOR_V
#    define PLOOPY_DRAGSCROLL_DIVISOR_V 8.0
#endif

static bool  dragscroll_active    = false;
static float scroll_accumulated_h = 0;
static float scroll_accumulated_v = 0;

void dragscroll_set(bool active) {
    dragscroll_active = active;
}

void dragscroll_toggle(void) {
    dragscroll_active ^= 1;
}

void dragscroll_task(report_mouse_t* report) {
    if (!dragscroll_active) {
        return;
    }

    float divisor_h = PLOOPY_DRAGSCROLL_DIVISOR_H;
    float divisor_v = PLOOPY_DRAGSCROLL_DIVISOR_V;

    /* Other hosts (e.g. macOS) ignore the HID resolution multiplier and take
       each count as a full notch. Same check as mode_wheels_default(). */
    os_variant_t host_os = detected_host_os();
    if (host_os != OS_WINDOWS && host_os != OS_LINUX) {
        divisor_h *= pointing_device_get_hires_scroll_resolution();
        divisor_v *= pointing_device_get_hires_scroll_resolution();
    }

    scroll_accumulated_h += (float)report->x / divisor_h;
    scroll_accumulated_v += (float)report->y / divisor_v;

    /* ploopyco.c casts to int8_t, which wraps on a fast roll. The A+ has
       WHEEL_EXTENDED_REPORT, so the report takes 16 bits. */
    mouse_hv_report_t h = (mouse_hv_report_t)scroll_accumulated_h;
    mouse_hv_report_t v = (mouse_hv_report_t)scroll_accumulated_v;
    scroll_accumulated_h -= h;
    scroll_accumulated_v -= v;

    report->h = h;
#ifdef PLOOPY_DRAGSCROLL_INVERT
    report->v = -v;
#else
    report->v = v;
#endif

    report->x = 0;
    report->y = 0;
}
