// SPDX-License-Identifier: MIT — see LICENSE file

#ifndef UI_RENDER_HELPER_H
#define UI_RENDER_HELPER_H

#include "renderer.h"
#include "keyboard.h"
#include "colors.h"

// Draw the keyboard (keys + background)
#include "ui.h" // For KeyVisualMetadata

void ui_render_draw_keyboard(Renderer *renderer, Keyboard *keyboard,
                            Rectangle *key_bounds, KeyVisualMetadata *key_metadata,
                            int key_count, int win_width, int win_height,
                            int menu_offset, double opacity,
                            ColorScheme scheme,
                            const char *font_family,
                            bool draw_dynamic);

// Draw the menu bar
void ui_render_draw_menu_bar(Renderer *renderer, UI *ui,
                            double opacity, int font_size,
                            ColorScheme scheme);

// Draw the drag handle pill
void ui_render_draw_drag_handle(Renderer *renderer, int win_width,
                               ColorScheme scheme, double opacity);

#endif // UI_RENDER_HELPER_H
