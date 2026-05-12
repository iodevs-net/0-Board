// SPDX-License-Identifier: MIT — see LICENSE file

#include "ui_render_helper.h"
#include "ui_internal.h"
#include "constants.h"
#include "layout.h"
#define XK_MISCELLANY
#include <X11/keysym.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

// Determine if a key label is a single unicode symbol (modifier icons)


static Color color_with_opacity(Color c, double opacity) {
    return (Color){c.red, c.green, c.blue, c.alpha * opacity};
}

/**
 * ui_render_draw_keyboard: The core rendering logic.
 * 
 * STRATEGY (Dual-Pass / Zero-Resource):
 * 1. Static Pass (draw_dynamic = false):
 *    Renders keys in their normal state. This is called once per layout/layer change
 *    to populate a background cache (Pixmaps). 
 * 
 * 2. Dynamic Pass (draw_dynamic = true):
 *    Renders ONLY the keys that are currently pressed or active (Modifiers).
 *    This overlay is drawn every frame (30fps) on top of the static cache.
 * 
 * This separation allows us to use expensive BitBlt operations for the bulk
 * of the UI, while keeping high responsiveness for interactions.
 */
void ui_render_draw_keyboard(Renderer *renderer, Keyboard *keyboard,
                            Rectangle *key_bounds, KeyVisualMetadata *key_metadata,
                            int key_count, int win_width, int win_height,
                            int menu_offset, double opacity,
                            ColorScheme scheme,
                            const char *font_family,
                            bool draw_dynamic) {
    if (!renderer || !keyboard || !key_bounds || !key_metadata) return;

    Layout *layout = keyboard_get_layout(keyboard);
    KeyboardState state = keyboard_get_state(keyboard);

    // 1. Background (Only if not drawing dynamic overlay)
    if (!draw_dynamic) {
        Rectangle bg = {0, menu_offset, win_width, win_height - menu_offset};
        renderer_draw_rectangle(renderer, bg,
            color_with_opacity(scheme.background, opacity), KEYBOARD_CORNER_RADIUS);
    }

    for (int i = 0; i < key_count && i < layout->num_keys; i++) {
        KeyDef *key = &layout->keys[i];
        KeyVisualMetadata *meta = &key_metadata[i];
        Rectangle kb = key_bounds[i];

        // Modifier and pressed states are only relevant for the dynamic pass
        bool is_active_modifier = false;
        bool is_pressed = false;

        if (draw_dynamic) {
            if (key->flags & KEYFLAG_SHIFT)
                is_active_modifier = (state.active_layer == KEYBOARD_LAYER_SHIFT);
            else if (key->flags & KEYFLAG_CTRL)
                is_active_modifier = (state.ctrl_state != KBD_MODIFIER_OFF);
            else if (key->flags & KEYFLAG_ALT)
                is_active_modifier = (state.alt_state != KBD_MODIFIER_OFF);
            else if (key->flags & KEYFLAG_META)
                is_active_modifier = (state.meta_state != KBD_MODIFIER_OFF);
            else if (key->normal == XK_Caps_Lock)
                is_active_modifier = state.caps_lock;
            
            is_pressed = (state.pressed_key_index == i);
        }
        bool is_recording = false;
        const char *label = keyboard_get_key_label(keyboard, i);
        if (label && strcmp(label, "mic") == 0 && access("/tmp/0-voice-recording", F_OK) == 0) {
            is_recording = true;
        }

        // Filtering: 
        // - Background pass (draw_dynamic=false): Draw EVERY key in its current label/state.
        // - Dynamic pass (draw_dynamic=true): Only draw keys that need a highlight.
        if (draw_dynamic && !(is_pressed || is_active_modifier || is_recording)) continue;

        // --- Key color selection (Using Enhanced Metadata) ---
        Color key_color;
        Color apple_orange = {1.0, 0.58, 0.0, 1.0}; // #FF9500
        
        bool is_shift_key = (key->flags & KEYFLAG_SHIFT) != 0;
        bool is_ctrl_key  = (key->flags & KEYFLAG_CTRL)  != 0;
        bool is_alt_key   = (key->flags & KEYFLAG_ALT)   != 0;
        bool is_meta_key  = (key->flags & KEYFLAG_META)  != 0;
        bool is_caps_lock = (key->normal == XK_Caps_Lock);

        if (is_pressed) {
            key_color = scheme.key_pressed;
        } else if (label && strcmp(label, "mic") == 0 && access("/tmp/0-voice-recording", F_OK) == 0) {
            key_color = (Color){0.9, 0.2, 0.2, 1.0}; // Red when recording
        } else if (is_active_modifier) {
            bool is_locked = false;
            if (is_shift_key) is_locked = state.shift_locked;
            else if (is_ctrl_key) is_locked = (state.ctrl_state == KBD_MODIFIER_LOCKED);
            else if (is_alt_key)  is_locked = (state.alt_state  == KBD_MODIFIER_LOCKED);
            else if (is_meta_key) is_locked = (state.meta_state == KBD_MODIFIER_LOCKED);
            else if (is_caps_lock) is_locked = state.caps_lock;

            if (is_locked) {
                // Locked modifier: Fill background with Orange
                key_color = apple_orange;
            } else {
                // One-Shot modifier: Keep background normal (gray), outline will be active color
                key_color = scheme.key_modifier;
            }
        } else if (meta->is_special) {
            key_color = scheme.key_special;
        } else if (meta->is_modifier) {
            key_color = scheme.key_modifier;
        } else if (meta->is_number) {
            key_color = scheme.key_number;
        } else if (meta->is_text) {
            key_color = scheme.key_text;
        } else {
            key_color = scheme.key_normal;
        }

        // 2. Key shadow
        Rectangle shadow_rect = {kb.x, kb.y + KEY_SHADOW_OFFSET, kb.width, kb.height};
        renderer_draw_rectangle(renderer, shadow_rect,
            color_with_opacity(scheme.key_shadow, opacity), KEY_CORNER_RADIUS);

        // 3. Key body
        renderer_draw_rectangle(renderer, kb,
            color_with_opacity(key_color, opacity), KEY_CORNER_RADIUS);

        // 4. Active modifier accent border
        if (is_active_modifier && !is_pressed) {
            bool is_locked = false;
            if (is_shift_key) is_locked = state.shift_locked;
            else if (is_ctrl_key) is_locked = (state.ctrl_state == KBD_MODIFIER_LOCKED);
            else if (is_alt_key)  is_locked = (state.alt_state  == KBD_MODIFIER_LOCKED);
            else if (is_meta_key) is_locked = (state.meta_state == KBD_MODIFIER_LOCKED);
            else if (is_caps_lock) is_locked = state.caps_lock;

            Color outline_color = is_locked ? apple_orange : scheme.accent;
            renderer_draw_rectangle_outline(renderer, kb,
                color_with_opacity(outline_color, opacity * 0.9),
                2.0, KEY_CORNER_RADIUS);
        }

        // 5. Key label (Using Metadata)
        if (label && label[0] != '\0') {
            FontSpec font = {
                font_family ? font_family : "Inter",
                meta->font_size, meta->bold, false
            };
            renderer_draw_text(renderer, label, kb, font, 
                              color_with_opacity(scheme.text_primary, opacity),
                              ALIGN_CENTER, VALIGN_CENTER);
        }
    }
}

void ui_render_draw_menu_bar(Renderer *renderer, UI *ui,
                            double opacity, int font_size,
                            ColorScheme scheme) {
    if (!renderer || !ui) return;

    Rectangle menu_bar = {0, 0, ui->current_width, MENU_BAR_HEIGHT};
    Color menu_bg = color_with_opacity(scheme.background, opacity);
    // Deep Space Gray for the bar
    menu_bg.red *= 0.8; menu_bg.green *= 0.8; menu_bg.blue *= 0.8;
    renderer_draw_rectangle(renderer, menu_bar, menu_bg, 10.0); // Rounded top bar

    FontSpec font = {"Inter", (int)(font_size * 0.65), false, false};
    FontSpec font_branding = {"Inter", (int)(font_size * 0.50), false, false};
    Color text_color = color_with_opacity(scheme.text_primary, opacity);
    Color pill_color = color_with_opacity(scheme.key_modifier, opacity * 0.5);

    // 1. Branding (Centered)
    Rectangle branding_rect = {0, 0, ui->current_width, MENU_BAR_HEIGHT};
    renderer_draw_text(renderer, "0-Board by Leonardo Vergara - iodevs.net", 
                      branding_rect, font_branding, 
                      color_with_opacity(scheme.text_secondary, opacity * 0.7), 
                      ALIGN_CENTER, VALIGN_CENTER);

    // 2. Buttons
    // Button 0: Minus
    renderer_draw_rectangle(renderer, ui->menu_btn_bounds[0], pill_color, ui->menu_btn_bounds[0].height/2);
    renderer_draw_text(renderer, "−", ui->menu_btn_bounds[0], font, text_color, ALIGN_CENTER, VALIGN_CENTER);

    // Button 1: Plus
    renderer_draw_rectangle(renderer, ui->menu_btn_bounds[1], pill_color, ui->menu_btn_bounds[1].height/2);
    renderer_draw_text(renderer, "+", ui->menu_btn_bounds[1], font, text_color, ALIGN_CENTER, VALIGN_CENTER);

    // Button 2: Theme (Palette Icon)
    Rectangle btn_theme = ui->menu_btn_bounds[2];
    renderer_draw_rectangle(renderer, btn_theme, pill_color, btn_theme.height/2);
    int dot_size = 4;
    int dots_y = btn_theme.y + (btn_theme.height - dot_size) / 2;
    renderer_draw_rectangle(renderer, (Rectangle){btn_theme.x + 6, dots_y, dot_size, dot_size}, (Color){0.9, 0.4, 0.4, opacity}, 2);
    renderer_draw_rectangle(renderer, (Rectangle){btn_theme.x + 14, dots_y, dot_size, dot_size}, (Color){0.4, 0.9, 0.4, opacity}, 2);
    renderer_draw_rectangle(renderer, (Rectangle){btn_theme.x + 22, dots_y, dot_size, dot_size}, (Color){0.4, 0.4, 0.9, opacity}, 2);

    // Button 3: Close (Red Circle)
    Rectangle btn_close = ui->menu_btn_bounds[3];
    Color apple_red = {1.0, 0.37, 0.34, opacity};
    renderer_draw_rectangle(renderer, btn_close, apple_red, btn_close.height/2);
}

void ui_render_draw_drag_handle(Renderer *renderer, int win_width,
                               ColorScheme scheme, double opacity) {
    (void)renderer; (void)win_width; (void)scheme; (void)opacity;
    // Drag handle removed as per user request for a cleaner look
}
void ui_render_touchpad(Renderer *renderer, Touchpad *tp, Rectangle bounds, ColorScheme scheme) {
    // Draw dark background
    renderer_draw_rectangle(renderer, bounds, scheme.background, 14.0);

    int pad = 10;

    // Exit button (top-right corner)
    tp->exit_w = 50;
    tp->exit_h = 28;
    tp->exit_x = bounds.x + bounds.width - tp->exit_w - pad;
    tp->exit_y = bounds.y + pad;
    Color exit_bg = {1.0, 0.37, 0.34, 0.8}; // semitransparent red
    renderer_draw_rectangle(renderer,
        (Rectangle){tp->exit_x, tp->exit_y, tp->exit_w, tp->exit_h},
        exit_bg, tp->exit_h / 2);
    FontSpec exit_font = {"Inter", 13, true, false};
    renderer_draw_text(renderer, "\u2B05",
        (Rectangle){tp->exit_x, tp->exit_y, tp->exit_w, tp->exit_h},
        exit_font, (Color){1,1,1,1}, ALIGN_CENTER, VALIGN_CENTER);
    int bar_w = 12;
    int bar_h = bounds.height - tp->btn_h - pad*3;

    // Scroll bar area (right edge)
    Rectangle scroll = {bounds.x + bounds.width - bar_w - pad, bounds.y + pad, bar_w, bar_h};
    Color scroll_color = scheme.key_modifier;
    scroll_color.alpha *= 0.5;
    renderer_draw_rectangle(renderer, scroll, scroll_color, 6.0);

    // Scroll bar indicator (pill in center)
    Color pill_color = scheme.text_secondary;
    pill_color.alpha *= 0.3;
    Rectangle pill = {scroll.x + 2, scroll.y + bar_h/3, bar_w - 4, bar_h/3};
    renderer_draw_rectangle(renderer, pill, pill_color, 3.0);

    // Buttons at bottom
    int btn_h = tp->btn_h;
    int btn_y = bounds.y + bounds.height - btn_h - pad;
    int btn_w = (bounds.width - pad*3) / 2;
    Color btn_color = scheme.key_modifier;

    // Left button
    Rectangle left_btn = {bounds.x + pad, btn_y, btn_w, btn_h};
    renderer_draw_rectangle(renderer, left_btn, btn_color, 8.0);

    // Right button
    Rectangle right_btn = {bounds.x + pad*2 + btn_w, btn_y, btn_w, btn_h};
    renderer_draw_rectangle(renderer, right_btn, btn_color, 8.0);

    // Button labels
    FontSpec font = {"Inter", 14, false, false};
    renderer_draw_text(renderer, "\u2190", left_btn, font, scheme.text_primary, ALIGN_CENTER, VALIGN_CENTER);
    renderer_draw_text(renderer, "\u2192", right_btn, font, scheme.text_primary, ALIGN_CENTER, VALIGN_CENTER);
}
