#include "ui_render_helper.h"
#include "constants.h"
#include "layout.h"
#define XK_MISCELLANY
#include <X11/keysym.h>
#include <stdio.h>
#include <string.h>

// Determine if a key label is a single unicode symbol (modifier icons)
static bool is_symbol_label(const char *label) {
    if (!label || label[0] == '\0') return false;
    // UTF-8 multi-byte sequences start with 0xC0+ byte
    return ((unsigned char)label[0] >= 0xC0 && strlen(label) <= 4);
}

static Color color_with_opacity(Color c, double opacity) {
    return (Color){c.red, c.green, c.blue, c.alpha * opacity};
}

void ui_render_draw_keyboard(Renderer *renderer, Keyboard *keyboard,
                            Rectangle *key_bounds, int key_count,
                            int win_width, int win_height,
                            int menu_offset, double opacity,
                            int color_scheme_index,
                            const char *font_family) {
    if (!renderer || !keyboard) return;

    ColorScheme scheme = color_scheme_get(color_scheme_index);
    double base_font_size = win_width * FONT_SIZE_RATIO + 2;

    // 1. Background
    Rectangle bg = {0, menu_offset, win_width, win_height - menu_offset};
    renderer_draw_rectangle(renderer, bg,
        color_with_opacity(scheme.background, opacity), KEYBOARD_CORNER_RADIUS);

    if (!key_bounds || key_count <= 0) return;

    Layout *layout = keyboard_get_layout(keyboard);
    KeyboardState state = keyboard_get_state(keyboard);

    for (int i = 0; i < key_count && i < layout->num_keys; i++) {
        KeyDef *key = &layout->keys[i];
        Rectangle kb = key_bounds[i];

        // --- Key color selection ---
        Color key_color;
        bool is_modifier = (key->flags & KEYFLAG_MODIFIER);

        if (is_modifier)
            key_color = scheme.key_modifier;
        else
            key_color = scheme.key_normal;

        // Special keys: backspace, return, space
        if (key->normal == XK_BackSpace || key->normal == XK_Return ||
            key->normal == XK_space)
            key_color = scheme.key_special;

        // Active modifier highlight
        bool is_active_modifier = false;
        if (key->flags & KEYFLAG_SHIFT)
            is_active_modifier = (state.current_layer == KEYBOARD_LAYER_SHIFT);
        else if (key->normal == XK_Caps_Lock)
            is_active_modifier = state.caps_lock;

        // Pressed state
        bool is_pressed = (state.pressed_key_index == i);
        if (is_pressed)
            key_color = scheme.key_pressed;

        // 2. Key shadow (1px below, slightly darker)
        Rectangle shadow_rect = {kb.x, kb.y + KEY_SHADOW_OFFSET,
                                 kb.width, kb.height};
        renderer_draw_rectangle(renderer, shadow_rect,
            color_with_opacity(scheme.key_shadow, opacity), KEY_CORNER_RADIUS);

        // 3. Key body
        renderer_draw_rectangle(renderer, kb,
            color_with_opacity(key_color, opacity), KEY_CORNER_RADIUS);

        // 4. Active modifier accent border
        if (is_active_modifier && !is_pressed) {
            renderer_draw_rectangle_outline(renderer, kb,
                color_with_opacity(scheme.accent, opacity * 0.8),
                2.0, KEY_CORNER_RADIUS);
        }

        // 5. Key label
        const char *label = keyboard_get_key_label(keyboard, i);
        if (label && label[0] != '\0') {
            Color text_color = color_with_opacity(scheme.text_primary, opacity);

            // Scale font based on key type
            double font_size;
            bool bold = false;
            if (is_symbol_label(label)) {
                // Unicode symbols (⇧⌫⏎ etc.) — slightly larger
                font_size = base_font_size * LABEL_SCALE_SYMBOL;
            } else if (is_modifier && strlen(label) > 1) {
                // Text modifiers (ctrl, alt, fn) — smaller
                font_size = base_font_size * LABEL_SCALE_MODIFIER;
            } else {
                // Regular letters and single chars
                font_size = base_font_size * LABEL_SCALE_LETTER;
                bold = true;
            }

            FontSpec font = {
                font_family ? font_family : "Inter",
                font_size, bold, false
            };
            renderer_draw_text(renderer, label, kb, font, text_color,
                              ALIGN_CENTER, VALIGN_CENTER);
        }
    }
}

void ui_render_draw_menu_bar(Renderer *renderer, int win_width,
                            double opacity, int font_size,
                            int color_scheme_index) {
    if (!renderer) return;

    ColorScheme scheme = color_scheme_get(color_scheme_index);

    Rectangle menu_bar = {0, 0, win_width, MENU_BAR_HEIGHT};
    Color menu_color = color_with_opacity(scheme.background, opacity);
    renderer_draw_rectangle(renderer, menu_bar, menu_color, 0);

    // Divider line at bottom of menu
    Rectangle divider = {0, MENU_BAR_HEIGHT - 1, win_width, 1};
    Color div_color = color_with_opacity(scheme.key_modifier, opacity * 0.5);
    renderer_draw_rectangle(renderer, divider, div_color, 0);

    char menu_text[256];
    snprintf(menu_text, sizeof(menu_text),
            "[-]  opacity  [+]     [theme]");

    FontSpec font = {
        "Inter", font_size * 0.65, false, false
    };
    Color text_color = color_with_opacity(scheme.text_secondary, opacity);
    Rectangle text_bounds = {20, 0, win_width - 40, MENU_BAR_HEIGHT};
    renderer_draw_text(renderer, menu_text, text_bounds, font, text_color,
                      ALIGN_LEFT, VALIGN_CENTER);
}

void ui_render_draw_drag_handle(Renderer *renderer, int win_width,
                               int color_scheme_index, double opacity) {
    if (!renderer) return;

    ColorScheme scheme = color_scheme_get(color_scheme_index);

    double pill_x = (win_width - DRAG_PILL_WIDTH) / 2.0;
    double pill_y = 6.0;
    Rectangle pill = {pill_x, pill_y, DRAG_PILL_WIDTH, DRAG_PILL_HEIGHT};
    renderer_draw_rectangle(renderer, pill,
        color_with_opacity(scheme.drag_handle, opacity),
        DRAG_PILL_HEIGHT / 2.0);
}
