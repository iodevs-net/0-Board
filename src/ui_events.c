#include "ui_events.h"
#include "ui_internal.h"
#include "layout_engine.h"
#include "colors.h"
#include "debug.h"
#include <X11/keysym.h>
#include <string.h>

void ui_handle_button_press(UI *ui, int wx, int wy, int rx, int ry, int button) {
    if (!ui) return;

    // Drag: button 1 in top drag handle zone, or middle-click anywhere
    int drag_zone = ui->current_height * DRAG_HANDLE_HEIGHT_RATIO;
    if (drag_zone < 12) drag_zone = 12;

    int keyboard_top = ui->menu_visible ? MENU_BAR_HEIGHT : 0;

    if (button == 2 || (button == 1 && wy >= keyboard_top && wy < keyboard_top + drag_zone)) {
        x11_window_get_position(ui->window, &ui->win_start_x, &ui->win_start_y);
        ui->drag_start_root_x = rx;
        ui->drag_start_root_y = ry;
        ui->dragging = true;
        return;
    }

    if (button != 1) return;

    int mx = wx;
    int my = wy;

    // Menu bar buttons — proportional hitboxes
    if (my < MENU_BAR_HEIGHT && ui->menu_visible) {
        int zone_w = ui->current_width / 6;
        if (mx < zone_w) {
            ui_set_opacity(ui, ui->opacity - MENU_OPACITY_STEP);
        } else if (mx < zone_w * 2) {
            ui_set_opacity(ui, ui->opacity + MENU_OPACITY_STEP);
        } else if (mx > ui->current_width - zone_w * 2) {
            ui_set_color_scheme(ui, (ui->color_scheme_index + 1) % NUM_COLOR_SCHEMES);
        }
        return;
    }

    // Check keys
    for (int i = 0; i < ui->key_count; i++) {
        Rectangle *kb = &ui->key_bounds[i];
        if (mx >= kb->x && mx <= kb->x + kb->width &&
            my >= kb->y && my <= kb->y + kb->height) {

            const char *label = keyboard_get_key_label(ui->keyboard, i);

            // "fn" key toggles menu
            if (label && strcmp(label, "fn") == 0) {
                if (ui->menu_visible) ui_hide_menu(ui);
                else ui_show_menu(ui);
            } else if (ui->engine) {
                KeySym sym = keyboard_get_keysym(ui->keyboard, i);
                int effective_layer = keyboard_get_effective_layer(ui->keyboard, i);

                if (sym != 0) {
                    keyboard_press_key(ui->keyboard, i);

                    int modifiers = (effective_layer == 1) ? 1 : 0;
                    engine_send_key_ex(ui->engine, sym, true, modifiers);
                    engine_send_key_ex(ui->engine, sym, false, modifiers);
                    engine_flush(ui->engine);

                    keyboard_notify_key_sent(ui->keyboard, i);
                }
            }
            break;
        }
    }
}

void ui_handle_button_release(UI *ui, int x, int y, int button) {
    if (!ui || button != 1) return;
    (void)x; (void)y;

    KeyboardState state = keyboard_get_state(ui->keyboard);
    if (state.pressed_key_index != -1) {
        keyboard_release_key(ui->keyboard, state.pressed_key_index);
    }
}

void ui_handle_motion(UI *ui, int rx, int ry) {
    if (!ui || !ui->dragging || !ui->window) return;

    int dx = rx - ui->drag_start_root_x;
    int dy = ry - ui->drag_start_root_y;

    x11_window_move(ui->window, ui->win_start_x + dx, ui->win_start_y + dy);
}

void ui_event_callback(X11Window *window, WindowEvent *event, void *user_data) {
    UI *ui = (UI*)user_data;
    if (!ui || !event) return;
    (void)window;

    switch (event->type) {
        case WINDOW_EVENT_RESIZE:
            if (event->width != ui->current_width || event->height != ui->current_height) {
                ui->current_width = event->width;
                ui->current_height = event->height;
                ui_calculate_layout(ui);
                ui->dirty = true;
            }
            break;

        case WINDOW_EVENT_MOTION:
            ui_handle_motion(ui, event->root_x, event->root_y);
            break;

        case WINDOW_EVENT_BUTTON_PRESS:
            ui_handle_button_press(ui, event->x, event->y,
                event->root_x, event->root_y, (int)event->button);
            break;

        case WINDOW_EVENT_BUTTON_RELEASE:
            ui->dragging = false;
            ui_handle_button_release(ui, event->x, event->y, (int)event->button);
            break;

        case WINDOW_EVENT_CLOSE:
            ui->should_close = true;
            break;

        default:
            break;
    }
}
