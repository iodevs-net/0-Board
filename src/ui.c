#include "ui_internal.h"
#include "ui_events.h"
#include "ui_render_helper.h"
#include "layout_engine.h"
#include "x11_cairo_bridge.h"
#include <stdlib.h>
#include <string.h>

void ui_calculate_layout(UI *ui) {
    if (!ui || !ui->keyboard) return;

    Layout *layout = keyboard_get_layout(ui->keyboard);
    Display *dpy = x11_window_get_display(ui->window);
    int screen_w = DisplayWidth(dpy, DefaultScreen(dpy));
    int screen_h = DisplayHeight(dpy, DefaultScreen(dpy));

    double ratios[] = {SCREEN_WIDTH_RATIO_SMALL, SCREEN_WIDTH_RATIO_MEDIUM, SCREEN_WIDTH_RATIO_LARGE};
    int kb_width = screen_w * ratios[ui->size_index];
    int kb_height = kb_width * KEYBOARD_HEIGHT_RATIO;
    int menu_offset = ui->menu_visible ? MENU_BAR_HEIGHT : 0;

    ui->current_width = kb_width;
    ui->current_height = kb_height + menu_offset;

    if (ui->key_bounds) free(ui->key_bounds);
    ui->key_count = layout->num_keys;
    ui->key_bounds = malloc(sizeof(Rectangle) * ui->key_count);

    layout_engine_calculate(layout, ui->current_width, ui->current_height,
                           menu_offset, ui->key_bounds);

    x11_window_resize(ui->window, ui->current_width, ui->current_height);
    renderer_resize(ui->renderer, ui->current_width, ui->current_height);

    // Auto-position: centered horizontally, docked to bottom
    int pos_x = (screen_w - ui->current_width) / 2;
    int pos_y = screen_h - ui->current_height;
    x11_window_move(ui->window, pos_x, pos_y);

    ui->dirty = true;
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
    x11_window_set_event_callback(ui->window, ui_event_callback, ui);
    x11_window_show(ui->window);
    x11_window_set_always_on_top(ui->window, true);

    return ui;
}

void ui_set_engine(UI *ui, Engine *engine) {
    if (ui) ui->engine = engine;
}

void ui_run(UI *ui) {
    if (!ui) return;

    while (!ui->should_close) {
        bool had_events = x11_window_wait_event(ui->window, 100);

        if (ui->dirty || keyboard_is_dirty(ui->keyboard) || had_events) {
            renderer_begin_frame(ui->renderer);
            renderer_clear(ui->renderer, (Color){0,0,0,CLEAR_ALPHA});

            int menu_offset = ui->menu_visible ? MENU_BAR_HEIGHT : 0;
            const char *current_font = font_manager_get_current_family(ui->font_manager);

            ui_render_draw_keyboard(ui->renderer, ui->keyboard,
                ui->key_bounds, ui->key_count,
                ui->current_width, ui->current_height,
                menu_offset, ui->opacity, ui->color_scheme_index,
                current_font);

            // Drag handle pill on top of keyboard
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
    if (!ui || scheme_index < 0 || scheme_index >= NUM_COLOR_SCHEMES) return;
    ui->color_scheme_index = scheme_index;
    ui->dirty = true;
}

const char* ui_get_font_family(const UI *ui) {
    return ui ? font_manager_get_current_family(ui->font_manager) : NULL;
}

bool ui_set_font_family(UI *ui, const char *family) {
    if (!ui || !font_manager_set_family(ui->font_manager, family)) return false;
    ui->dirty = true;
    ui_calculate_layout(ui);
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
bool ui_is_menu_visible(const UI *ui) { return ui ? ui->menu_visible : false; }
void ui_mark_dirty(UI *ui) { if (ui) ui->dirty = true; }
bool ui_is_dirty(const UI *ui) { return ui ? ui->dirty : false; }

void ui_destroy(UI *ui) {
    if (!ui) return;
    if (ui->key_bounds) free(ui->key_bounds);
    free(ui);
}