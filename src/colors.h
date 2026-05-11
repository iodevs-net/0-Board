// SPDX-License-Identifier: MIT — see LICENSE file

#ifndef COLORS_H
#define COLORS_H

#include "renderer.h"

// Color scheme definition — real RGBA, not grayscale
typedef struct {
    Color background;
    Color text_primary;
    Color text_secondary;
    
    // Key colors
    Color key_normal;
    Color key_pressed;
    Color key_modifier;  // Shift, Ctrl, Alt, Caps (Dark gray usually)
    Color key_text;      // Letters (Main alpha)
    Color key_number;    // Numbers row
    Color key_special;   // Enter, Space, Backspace, Arrows
    
    Color shift_active;  // Highlight for active shift
    Color accent;
    Color drag_handle;
    Color key_shadow;
} ColorScheme;

typedef enum {
    COLOR_SCHEME_SPACE_GRAY,
    COLOR_SCHEME_SILVER,
    COLOR_SCHEME_CUSTOM,
    NUM_COLOR_SCHEMES
} ColorSchemeId;

ColorScheme color_scheme_get(int id);
ColorScheme color_scheme_load_custom(const char *path);

#endif // COLORS_H