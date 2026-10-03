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

#pragma once

/* Drag scroll, in place of is_drag_scroll in keyboards/ploopyco/ploopyco.c.
   That version does not scale for hosts without hi-res scroll, and it runs
   in pointing_device_task_kb(), where the keymap cannot correct it. If
   upstream adds the scaling, delete this and use is_drag_scroll again. */
void dragscroll_set(bool active);
void dragscroll_toggle(void);

/* Turn ball motion into scroll while drag scroll is active. */
void dragscroll_task(report_mouse_t* report);
