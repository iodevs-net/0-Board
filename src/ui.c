#include "ui_internal.h"
#include <X11/keysym.h>
#include "ui_events.h"
#include "ui_render_helper.h"
#include "layout_engine.h"
#include "x11_cairo_bridge.h"
#include <stdlib.h>
#include <string.h>

// Minimum interval between frames (ms). 30fps max = ~33ms
#define MIN_FRAME_INTERVAL_MS 33

void ui_calculate_layout(UI *ui) {
    if (!ui || !ui->keyboard) return;

    Layout *layout = keyboard_get_layout(ui->keyboard);
    Display *dpy = x11_window_get_display(ui->window);
    int screen_w = DisplayWidth(dpy, DefaultScreen(dpy));

    double ratios[] = {SCREEN_WIDTH_RATIO_SMALL, SCREEN_WIDTH_RATIO_MEDIUM, SCREEN_WIDTH_RATIO_LARGE};
    int kb_width = screen_w * ratios[ui->size_index];
    int kb_height = kb_width * KEYBOARD_HEIGHT_RATIO;
    int menu_offset = ui->menu_visible ? MENU_BAR_HEIGHT : 0;

    ui->current_width = kb_width;
    ui->current_height = kb_height + menu_offset;

    if (ui->key_bounds) free(ui->key_bounds);
    if (ui->key_metadata) free(ui->key_metadata);
    
    ui->key_count = layout->num_keys;
    ui->key_bounds = malloc(sizeof(Rectangle) * ui->key_count);
    ui->key_metadata = malloc(sizeof(KeyVisualMetadata) * ui->key_count);

    layout_engine_calculate(layout, ui->current_width, ui->current_height,
                           menu_offset, ui->key_bounds);

    // Pre-calculate visual metadata
    double base_font_size = ui->current_width * FONT_SIZE_RATIO + 2;
    for (int i = 0; i < ui->key_count; i++) {
        KeyDef *key = &layout->keys[i];
        KeyVisualMetadata *m = &ui->key_metadata[i];
        
        m->is_modifier = (key->flags & KEYFLAG_MODIFIER);
        m->is_special = (key->normal == XK_BackSpace || key->normal == XK_Return || key->normal == XK_space);
        
        const char *label = key->label;
        m->is_symbol = (label && (unsigned char)label[0] >= 0xC0 && strlen(label) <= 4);
        
        if (m->is_symbol) {
            m->font_size = base_font_size * LABEL_SCALE_SYMBOL;
            m->bold = false;
        } else if (m->is_modifier && label && strlen(label) > 1) {
            m->font_size = base_font_size * LABEL_SCALE_MODIFIER;
            m->bold = false;
        } else {
            m->font_size = base_font_size * LABEL_SCALE_LETTER;
            m->bold = true;
        }
    }



    ui->bg_dirty = true;
    ui->dirty = true;
}

void ui_apply_geometry(UI *ui, int x, int y) {
    if (!ui) return;
    x11_window_move_resize(ui->window, x, y, ui->current_width, ui->current_height);
    renderer_resize(ui->renderer, ui->current_width, ui->current_height);
}

UI* ui_create(UIConfig *config, Keyboard *keyboard,
              Renderer *renderer, X11Window *window,
              FontManager *font_manager) {
    UI *ui = calloc(1, sizeof(UI));
    if (!ui) return NULL;

    ui->keyboard = keyboard;
    ui->renderer = renderer;
    ui->window = window;
    ui->font_manager = font_manager;

    if (config) ui->config = *config;
    else {
        ui->config.initial_opacity = 0.94;
        ui->config.initial_size = 1;
    }

    ui->opacity = ui->config.initial_opacity;
    ui->size_index = ui->config.initial_size;
    ui->menu_visible = ui->config.show_menu_bar;
    ui->dirty = true;

    ui_calculate_layout(ui);

    // Initial position: centered horizontally, docked to bottom
    Display *dpy = x11_window_get_display(ui->window);
    int screen_w = DisplayWidth(dpy, DefaultScreen(dpy));
    int screen_h = DisplayHeight(dpy, DefaultScreen(dpy));
    int pos_x = (screen_w - ui->current_width) / 2;
    int pos_y = screen_h - ui->current_height;
    x11_window_move(ui->window, pos_x, pos_y);

    x11_window_set_event_callback(ui->window, ui_event_callback, ui);
    x11_window_show(ui->window);
    x11_window_set_always_on_top(ui->window, true);

    return ui;
}

void ui_set_engine(UI *ui, Engine *engine) {
    if (ui) ui->engine = engine;
}

static void ui_render_static_bg_callback(Renderer *renderer, void *user_data) {
    UI *ui = (UI*)user_data;
    renderer_clear(renderer, (Color){0,0,0,0});
    int menu_offset = ui->menu_visible ? MENU_BAR_HEIGHT : 0;
    const char *current_font = font_manager_get_current_family(ui->font_manager);

    ui_render_draw_keyboard(renderer, ui->keyboard,
        ui->key_bounds, ui->key_metadata, ui->key_count,
        ui->current_width, ui->current_height,
        menu_offset, 1.0, ui->color_scheme_index,
        current_font, false); // draw_dynamic = false
}

static void ui_update_bg_cache(UI *ui) {
    if (ui->bg_cache) renderer_destroy_surface(ui->bg_cache);
    ui->bg_cache = renderer_create_surface(ui->renderer, ui->current_width, ui->current_height);
    renderer_draw_to_surface(ui->renderer, ui->bg_cache, ui_render_static_bg_callback, ui);
    ui->bg_dirty = false;
}

static void ui_render_frame(UI *ui) {
    KeyboardState kb_state = keyboard_get_state(ui->keyboard);
    bool layer_changed = ((int)kb_state.current_layer != ui->last_rendered_layer) || 
                         (kb_state.caps_lock != ui->last_rendered_caps);

    if (ui->bg_dirty || layer_changed || !ui->bg_cache) {
        ui->last_rendered_layer = kb_state.current_layer;
        ui->last_rendered_caps = kb_state.caps_lock;
        ui_update_bg_cache(ui);
    }

    renderer_begin_frame(ui->renderer);
    renderer_clear(ui->renderer, (Color){0,0,0,CLEAR_ALPHA});

    // 1. Draw cached background
    renderer_draw_surface(ui->renderer, ui->bg_cache, 0, 0, ui->opacity);

    // 2. Draw dynamic overlay (pressed keys, active modifiers)
    int menu_offset = ui->menu_visible ? MENU_BAR_HEIGHT : 0;
    const char *current_font = font_manager_get_current_family(ui->font_manager);

    ui_render_draw_keyboard(ui->renderer, ui->keyboard,
        ui->key_bounds, ui->key_metadata, ui->key_count,
        ui->current_width, ui->current_height,
        menu_offset, ui->opacity, ui->color_scheme_index,
        current_font, true); // draw_dynamic = true

    ui_render_draw_drag_handle(ui->renderer, ui->current_width,
        ui->color_scheme_index, ui->opacity);

    if (ui->menu_visible) {
        ui_render_draw_menu_bar(ui->renderer, ui->current_width,
            ui->opacity, ui->current_width * FONT_SIZE_RATIO + 2,
            ui->color_scheme_index);
    }

    renderer_end_frame(ui->renderer);
    renderer_present_to_window(ui->renderer);

    ui->dirty = false;
    keyboard_mark_clean(ui->keyboard);
}

void ui_run(UI *ui) {
    ui_run_with_shutdown(ui, NULL);
}

void ui_run_with_shutdown(UI *ui, volatile sig_atomic_t *shutdown_flag) {
    if (!ui) return;

    // Initial render
    ui_render_frame(ui);

    while (!ui->should_close) {
        // Check external shutdown flag (signals)
        if (shutdown_flag && *shutdown_flag) {
            ui->should_close = true;
            break;
        }

        bool needs_render = false;

        if (ui->dirty || keyboard_is_dirty(ui->keyboard)) {
            // Pending render — use short timeout so we batch events
            x11_window_wait_event(ui->window, MIN_FRAME_INTERVAL_MS);
            x11_window_process_events(ui->window);
            needs_render = true;
        } else {
            // Nothing pending — wait up to 500ms (allows signal checks)
            bool had_event = x11_window_wait_event(ui->window, 500);
            if (had_event) {
                x11_window_process_events(ui->window);
                needs_render = ui->dirty || keyboard_is_dirty(ui->keyboard);
            }
        }

        if (needs_render && !ui->should_close) {
            ui_render_frame(ui);
        }
    }
}

double ui_get_opacity(const UI *ui) { return ui ? ui->opacity : 0.0; }

void ui_set_opacity(UI *ui, double opacity) {
    if (!ui || opacity < MIN_OPACITY || opacity > MAX_OPACITY) return;
    ui->opacity = opacity;
    ui->dirty = true;
    x11_window_set_opacity(ui->window, opacity);
}

int ui_get_color_scheme(const UI *ui) { return ui ? ui->color_scheme_index : 0; }

void ui_set_color_scheme(UI *ui, int scheme_index) {
    if (!ui) return;
    ui->color_scheme_index = scheme_index;
    ui->bg_dirty = true;
    ui->dirty = true;
}

const char* ui_get_font_family(const UI *ui) {
    return ui ? font_manager_get_current_family(ui->font_manager) : NULL;
}

bool ui_set_font_family(UI *ui, const char *family) {
    if (!ui || !font_manager_set_family(ui->font_manager, family)) return false;
    
    int x, y;
    x11_window_get_position(ui->window, &x, &y);
    
    ui->dirty = true;
    ui_calculate_layout(ui);
    
    // Apply new size at current position
    ui_apply_geometry(ui, x, y);
    return true;
}

int ui_get_size_index(const UI *ui) { return ui ? ui->size_index : 1; }

void ui_set_size_index(UI *ui, int size_index) {
    if (!ui || size_index < 0 || size_index > 2) return;
    ui->size_index = size_index;
    ui_calculate_layout(ui);
}

void ui_show_menu(UI *ui) { if (ui) { ui->menu_visible = true; ui_calculate_layout(ui); } }
void ui_hide_menu(UI *ui) { if (ui) { ui->menu_visible = false; ui_calculate_layout(ui); } }

void ui_toggle_dock_position(UI *ui) {
    if (!ui || !ui->window) return;
    
    Display *dpy = x11_window_get_display(ui->window);
    int screen_w = DisplayWidth(dpy, DefaultScreen(dpy));
    int screen_h = DisplayHeight(dpy, DefaultScreen(dpy));
    
    ui->docked_top = !ui->docked_top;
    
    int pos_x = (screen_w - ui->current_width) / 2;
    int pos_y = ui->docked_top ? 0 : (screen_h - ui->current_height);
    
    x11_window_move(ui->window, pos_x, pos_y);
    ui->dirty = true;
}
bool ui_is_menu_visible(const UI *ui) { return ui ? ui->menu_visible : false; }
void ui_mark_dirty(UI *ui) { if (ui) ui->dirty = true; }
bool ui_is_dirty(const UI *ui) { return ui ? ui->dirty : false; }

void ui_destroy(UI *ui) {
    if (!ui) return;
    if (ui->key_bounds) free(ui->key_bounds);
    if (ui->key_metadata) free(ui->key_metadata);
    if (ui->bg_cache) renderer_destroy_surface(ui->bg_cache);
    free(ui);
}