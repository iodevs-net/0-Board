#ifndef UI_RENDER_HELPER_H
#define UI_RENDER_HELPER_H

#include "renderer.h"
#include "keyboard.h"
#include "colors.h"

// Draw the keyboard (keys + background)
void ui_render_draw_keyboard(Renderer *renderer, Keyboard *keyboard,
                            Rectangle *key_bounds, int key_count,
                            int win_width, int win_height,
                            int menu_offset, double opacity,
                            int color_scheme_index,
                            const char *font_family);

// Draw the menu bar
void ui_render_draw_menu_bar(Renderer *renderer, int win_width,
                            double opacity, int font_size,
                            int color_scheme_index);

// Draw the drag handle pill
void ui_render_draw_drag_handle(Renderer *renderer, int win_width,
                               int color_scheme_index, double opacity);

#endif // UI_RENDER_HELPER_H
