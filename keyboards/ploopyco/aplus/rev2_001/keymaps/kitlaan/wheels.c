/* Copyright 2023 Colin Lam (Ploopy Corporation)
 * Copyright 2020 Christopher Courtney, aka Drashna Jael're  (@drashna) <drashna@live.com>
 * Copyright 2019 Sunjun Kim
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

#include "printf.h"

#include "tmag5273wheel.h"

#include "kitlaan.h"
#include "wheels.h"

/* State variables for wheels. */
static uint16_t leftwheel_deadzone_center  = 0;
static uint16_t rightwheel_deadzone_center = 0;

static uint16_t leftwheel_current_position  = 0;
static uint16_t rightwheel_current_position = 0;
static uint32_t last_scroll_time            = 0;

void wheels_pre_init(void) {
    /* Set up GPIO for both TMAG sensors. */
    gpio_set_pin_output_push_pull(TMAG5273_D0_PWR_PIN);
    gpio_set_pin_output_push_pull(TMAG5273_D1_PWR_PIN);
    gpio_write_pin_low(TMAG5273_D0_PWR_PIN);
    gpio_write_pin_low(TMAG5273_D1_PWR_PIN);
}

void wheels_init(void) {
    /* ========================== Scroll Wheel Setup ======================= */

    /* Start up the two wheels. */
    tmag5273_init();

    /* Init TMAG5273 D0 (left wheel) */
    gpio_write_pin_high(TMAG5273_D0_PWR_PIN);
    wait_us(370); // TMAG tstart_power_up, plus 100us margin
    tmag5273_init_device(TMAG5273_D0_I2C_ADDRESS);
    wait_ms(20);
    leftwheel_current_position = tmag5273_get_angle(TMAG5273_D0_I2C_ADDRESS);
    leftwheel_deadzone_center  = leftwheel_current_position;

    /* Init TMAG5273 D1 (right wheel) */
    gpio_write_pin_high(TMAG5273_D1_PWR_PIN);
    wait_us(370); // TMAG tstart_power_up, plus 100us margin
    tmag5273_init_device(TMAG5273_D1_I2C_ADDRESS);
    wait_ms(20);
    rightwheel_current_position = tmag5273_get_angle(TMAG5273_D1_I2C_ADDRESS);
    rightwheel_deadzone_center  = rightwheel_current_position;
}

/* State variables for the scroll wheels. */
static int16_t leftwheel_lowres_scroll_tick  = 0;
static int16_t rightwheel_lowres_scroll_tick = 0;

static int16_t leftwheel_arrow_scroll_tick  = 0;
static int16_t rightwheel_arrow_scroll_tick = 0;

static int16_t leftwheel_volume_scroll_tick  = 0;
static int16_t rightwheel_volume_scroll_tick = 0;

static uint32_t leftwheel_timeout  = 0;
static uint32_t rightwheel_timeout = 0;

report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
    // throttle reads
    if (timer_elapsed32(last_scroll_time) > 10) {
        uint16_t leftwheel_rawangle  = tmag5273_get_angle(TMAG5273_D0_I2C_ADDRESS);
        uint16_t rightwheel_rawangle = tmag5273_get_angle(TMAG5273_D1_I2C_ADDRESS);

        int16_t leftwheel_deadzone_distance  = calculate_deadzone_distance(leftwheel_rawangle, &leftwheel_deadzone_center);
        int16_t rightwheel_deadzone_distance = calculate_deadzone_distance(rightwheel_rawangle, &rightwheel_deadzone_center);

        int16_t leftwheel_delta    = calculate_wheel_delta(leftwheel_rawangle, leftwheel_current_position);
        leftwheel_current_position = leftwheel_rawangle;

        int16_t rightwheel_delta    = calculate_wheel_delta(rightwheel_rawangle, rightwheel_current_position);
        rightwheel_current_position = rightwheel_rawangle;

        /* If either wheel moved, we reset the timeout counter. Else, we start
           counting inactivity. */
        if (abs(leftwheel_delta) > 2) {
            leftwheel_timeout = 0;
        } else {
            /* Don't increment forever, you'll get rollover. */
            if (leftwheel_timeout < TMAG5273_DEADZONE_PROTECTOR_TIMEOUT) {
                leftwheel_timeout += timer_elapsed32(last_scroll_time);
            }
        }
        if (abs(rightwheel_delta) > 2) {
            rightwheel_timeout = 0;
        } else {
            /* Don't increment forever, you'll get rollover. */
            if (rightwheel_timeout < TMAG5273_DEADZONE_PROTECTOR_TIMEOUT) {
                rightwheel_timeout += timer_elapsed32(last_scroll_time);
            }
        }

        /* Reset the counter so we know when to next perform all this scroll processing. */
        last_scroll_time = timer_read32();

        /* If either of the wheels times out, we move the deadzone window to
           where the position is. This prevents spurious scroll events. */
        if (leftwheel_timeout >= TMAG5273_DEADZONE_PROTECTOR_TIMEOUT && leftwheel_timeout < TMAG5273_DEADZONE_PROTECTOR_TIMEOUT * 2) {
            leftwheel_deadzone_center = leftwheel_current_position;
            dprintf("Left wheel timeout, deadzone moved to: %d\n", leftwheel_deadzone_center);

            /* Set the timeout to a large number so that we only shift the window once. */
            leftwheel_timeout = TMAG5273_DEADZONE_PROTECTOR_TIMEOUT * 4;
        }
        if (rightwheel_timeout >= TMAG5273_DEADZONE_PROTECTOR_TIMEOUT && rightwheel_timeout < TMAG5273_DEADZONE_PROTECTOR_TIMEOUT * 2) {
            rightwheel_deadzone_center = rightwheel_current_position;
            dprintf("Right wheel timeout, deadzone moved to: %d\n", rightwheel_deadzone_center);

            /* Set the timeout to a large number so that we only shift the window once. */
            rightwheel_timeout = TMAG5273_DEADZONE_PROTECTOR_TIMEOUT * 4;
        }

        /* If we're on the control layers, both wheels adjust the volume. */
        if (layer_state_is(LAYER_CONTROL)) {
            leftwheel_volume_scroll_tick += leftwheel_delta;
            rightwheel_volume_scroll_tick += rightwheel_delta;

            if (leftwheel_volume_scroll_tick > TMAG5273_VOLUME_SCROLL_TICK_SIZE) {
                tap_code(KC_VOLU);
                leftwheel_volume_scroll_tick = 0;
            } else if (leftwheel_volume_scroll_tick < -TMAG5273_VOLUME_SCROLL_TICK_SIZE) {
                tap_code(KC_VOLD);
                leftwheel_volume_scroll_tick = 0;
            }
            if (rightwheel_volume_scroll_tick > TMAG5273_VOLUME_SCROLL_TICK_SIZE) {
                tap_code(KC_VOLU);
                rightwheel_volume_scroll_tick = 0;
            } else if (rightwheel_volume_scroll_tick < -TMAG5273_VOLUME_SCROLL_TICK_SIZE) {
                tap_code(KC_VOLD);
                rightwheel_volume_scroll_tick = 0;
            }

            /* Return early -- no other scrolling activity happens in the
               control layer. */
            return mouse_report;
        }

        /* If scroll arrow mode is activated for horizontal scrolling, then we do that.
           This behaviour is the same across OSes. */
        if (user_config.horizontal_scroll_arrows) {
            leftwheel_arrow_scroll_tick += leftwheel_delta;
            rightwheel_arrow_scroll_tick += rightwheel_delta;

            if (rightwheel_arrow_scroll_tick > TMAG5273_HORIZ_SCROLL_TICK_SIZE) {
                tap_code(KC_RIGHT);
                rightwheel_arrow_scroll_tick = 0;
            } else if (rightwheel_arrow_scroll_tick < -TMAG5273_HORIZ_SCROLL_TICK_SIZE) {
                tap_code(KC_LEFT);
                rightwheel_arrow_scroll_tick = 0;
            }
            /* Set the delta to zero so we don't scroll *and* arrow at the same time. */
            rightwheel_delta = 0;
        }

        /* If scroll arrow mode is activated for vertical scrolling, then we do that.
           This behaviour is the same across OSes. */
        if (user_config.vertical_scroll_arrows) {
            leftwheel_arrow_scroll_tick += leftwheel_delta;
            rightwheel_arrow_scroll_tick += rightwheel_delta;

            if (leftwheel_arrow_scroll_tick > TMAG5273_VERT_SCROLL_TICK_SIZE) {
                tap_code(KC_DOWN);
                leftwheel_arrow_scroll_tick = 0;
            } else if (leftwheel_arrow_scroll_tick < -TMAG5273_VERT_SCROLL_TICK_SIZE) {
                tap_code(KC_UP);
                leftwheel_arrow_scroll_tick = 0;
            }
            /* Set the delta to zero so we don't scroll *and* arrow at the same time. */
            leftwheel_delta = 0;
        }

        /* If we're on Windows or Linux, send hi-res scroll events. */
        if ((detected_host_os() == OS_WINDOWS || detected_host_os() == OS_LINUX)) {
            if (leftwheel_deadzone_distance > (TMAG5273_WHEEL_DEADZONE - 10) || leftwheel_deadzone_distance < (-TMAG5273_WHEEL_DEADZONE + 10)) {
                mouse_report.v = -leftwheel_delta / TMAG5273_VERTICAL_WHEEL_SPEED_DIV;
            }
            if (rightwheel_deadzone_distance > (TMAG5273_WHEEL_DEADZONE - 10) || rightwheel_deadzone_distance < (-TMAG5273_WHEEL_DEADZONE + 10)) {
                if (!user_config.horizontal_scroll_arrows) {
                    mouse_report.h = rightwheel_delta / TMAG5273_HORIZONAL_WHEEL_SPEED_DIV;
                }
            }

        } else {
            /* In this case, we're on another OS, so we just send regular scroll events. */
            /* Certain operating systems, like MacOS, don't play well with the
               high-res scrolling implementation. For more details, see:
               https://github.com/qmk/qmk_firmware/issues/17585#issuecomment-2325248167
               128 gives the scroll wheels "ticks". */

            leftwheel_lowres_scroll_tick += leftwheel_delta;
            rightwheel_lowres_scroll_tick += rightwheel_delta;

            if (leftwheel_lowres_scroll_tick > TMAG5273_LOWRES_TICK_SIZE) {
                mouse_report.v               = -1;
                leftwheel_lowres_scroll_tick = 0;
            } else if (leftwheel_lowres_scroll_tick < -TMAG5273_LOWRES_TICK_SIZE) {
                mouse_report.v               = 1;
                leftwheel_lowres_scroll_tick = 0;
            }

            if (rightwheel_lowres_scroll_tick > TMAG5273_LOWRES_TICK_SIZE) {
                if (!user_config.horizontal_scroll_arrows) {
                    mouse_report.h = 1;
                }
                rightwheel_lowres_scroll_tick = 0;
            } else if (rightwheel_lowres_scroll_tick < -TMAG5273_LOWRES_TICK_SIZE) {
                if (!user_config.horizontal_scroll_arrows) {
                    mouse_report.h = -1;
                }
                rightwheel_lowres_scroll_tick = 0;
            }
        }

        /* Set scroll data to zero if the corresponding button is
           currently being held down. */

        /* Check left knob. */
        if (matrix_is_on(0, 6)) {
            mouse_report.v = 0;
        }
        /* Check right knob. */
        if (matrix_is_on(0, 7)) {
            mouse_report.h = 0;
        }
    }

    return mouse_report;
}
