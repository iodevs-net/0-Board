/*
 * 0-Board Virtual Keyboard
 * Copyright (c) 2026 Leonardo Vergara <leonardovergaramarin@gmail.com>
 * Licensed under the MIT License.
 */
#ifndef CONSTANTS_H
#define CONSTANTS_H

// Keyboard geometry
#define MAX_KEYS 128
#define MAX_ROWS 10

// Screen size ratios (keyboard width = screen width * ratio)
#define SCREEN_WIDTH_RATIO_SMALL  0.40
#define SCREEN_WIDTH_RATIO_MEDIUM 0.62
#define SCREEN_WIDTH_RATIO_LARGE  0.92

// Proportions derived from keyboard width
#define KEYBOARD_HEIGHT_RATIO 0.352
#define KEY_PADDING_RATIO     0.012
#define KEY_GAP_RATIO         0.004
#define FONT_SIZE_RATIO       0.030

// Font sizing per key type
#define LABEL_SCALE_LETTER   0.85
#define LABEL_SCALE_MODIFIER 0.70
#define LABEL_SCALE_SYMBOL   0.85

// Drag handle
#define DRAG_HANDLE_HEIGHT_RATIO 0.07
#define DRAG_PILL_WIDTH          40
#define DRAG_PILL_HEIGHT         4

// Menu bar
#define MENU_BAR_HEIGHT    36
#define MENU_OPACITY_STEP  0.1
#define MIN_OPACITY        0.3
#define MAX_OPACITY        1.0

// Rendering
#define KEYBOARD_CORNER_RADIUS 14.0
#define KEY_CORNER_RADIUS       6.0
#define KEY_SHADOW_OFFSET       1.5
#define KEY_SHADOW_ALPHA        0.25
#define CLEAR_ALPHA             0.0
#define NUM_BUFFERED_SIZES      3

// Font path (relative to binary)
#define FONT_PATH_PRIMARY   "assets/fonts/extras/ttf/Inter-Light.ttf"
#define FONT_PATH_FALLBACK  "assets/fonts/extras/ttf/Inter-Regular.ttf"

#endif // CONSTANTS_H