// SPDX-License-Identifier: MIT — see LICENSE file

#ifndef LAYOUT_ENGINE_H
#define LAYOUT_ENGINE_H

#include "layout.h"
#include "renderer.h"

/**
 * Calculates the bounding boxes for all keys in a layout.
 * 
 * @param layout The keyboard layout definition.
 * @param win_width Width of the available keyboard area.
 * @param win_height Height of the available keyboard area.
 * @param menu_offset Vertical offset if a menu bar is present.
 * @param out_bounds Array to store calculated rectangles (must be pre-allocated).
 * @return Number of keys processed.
 */
int layout_engine_calculate(const Layout *layout, int win_width, int win_height, 
                           int menu_offset, Rectangle *out_bounds);

#endif // LAYOUT_ENGINE_H
