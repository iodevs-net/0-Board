#include "ui_events.h"
#include "ui_internal.h"
#include "layout_engine.h"
#include "colors.h"
#include "debug.h"
#include <X11/keysym.h>
#include <string.h>
#include <unistd.h>

void ui_handle_button_press(UI *ui, int wx, int wy, int rx, int ry, int button) {
    if (!ui) return;

    // Drag zone: any edge of the keyboard
    int edge = ui->current_height * DRAG_HANDLE_HEIGHT_RATIO;
    if (edge < 12) edge = 12;

    int keyboard_top = ui->menu_visible ? MENU_BAR_HEIGHT : 0;
    bool on_edge = false;

    // Top edge
    if (wy >= keyboard_top && wy < keyboard_top + edge) on_edge = true;
    // Bottom edge
    if (wy > ui->current_height - edge) on_edge = true;
    // Left edge
    if (wx < edge) on_edge = true;
    // Right edge
    if (wx > ui->current_width - edge) on_edge = true;

    if (button == 2 || (button == 1 && on_edge)) {
        int win_x, win_y;
        x11_window_get_position(ui->window, &win_x, &win_y);
        ui->drag_offset_x = rx - win_x;
        ui->drag_offset_y = ry - win_y;
        ui->dragging = true;
        return;
    }

    if (button != 1) return;

    int mx = wx;
    int my = wy;

    // Menu bar buttons — proportional hitboxes
    if (my < MENU_BAR_HEIGHT && ui->menu_visible) {
        int zone_w = ui->current_width / 8; // Narrower zones
        if (mx < zone_w) {
            ui_set_opacity(ui, ui->opacity - MENU_OPACITY_STEP);
        } else if (mx < zone_w * 2) {
            ui_set_opacity(ui, ui->opacity + MENU_OPACITY_STEP);
        } else if (mx > zone_w * 3 && mx < zone_w * 5) {
            ui_set_color_scheme(ui, (ui->color_scheme_index + 1) % NUM_COLOR_SCHEMES);
        } else if (mx > ui->current_width - zone_w) {
            // Close button [x] at far right
            ui->should_close = true;
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
            } else if (label && strcmp(label, "↑↓") == 0) {
                ui_toggle_dock_position(ui);
            } else if (ui->engine) {
                KeySym sym = keyboard_get_keysym(ui->keyboard, i);
                
                if (sym != 0) {
                    // Right Command (⌘) toggles keyboard size
                    if (sym == XK_Super_R) {
                        // Capture anchor point (center of the size key) to keep pointer over it
                        int wx, wy;
                        x11_window_get_position(ui->window, &wx, &wy);
                        Rectangle old_k = ui->key_bounds[i];
                        
                        // Atomic anchor calculation (relative to screen)
                        double anchor_x = wx + old_k.x + old_k.width / 2.0;
                        double anchor_y = wy + old_k.y + old_k.height / 2.0;

                        int next_size = (ui->size_index + 1) % 3;
                        ui_set_size_index(ui, next_size); // Only calculates layout now

                        // Calculate new window position based on anchor
                        Rectangle new_k = ui->key_bounds[i];
                        int new_wx = (int)(anchor_x - (new_k.x + new_k.width / 2.0));
                        int new_wy = (int)(anchor_y - (new_k.y + new_k.height / 2.0));
                        
                        if (ui->docked_top) new_wy = 0;
                        
                        // Apply BOTH size and position in ONE atomic X11 call
                        ui_apply_geometry(ui, new_wx, new_wy);
                    } else {
                        keyboard_press_key(ui->keyboard, i);
                        
                        // Send the key via the engine
                        engine_send_key(ui->engine, sym, true);
                        usleep(10000); // 10ms delay for stability
                        engine_send_key(ui->engine, sym, false);
                        engine_flush(ui->engine);
                        
                        keyboard_notify_key_sent(ui->keyboard, i);
                    }
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

    x11_window_move(ui->window, rx - ui->drag_offset_x, ry - ui->drag_offset_y);
    // Do NOT set dirty — window move doesn't change content
}

void ui_event_callback(X11Window *window, WindowEvent *event, void *user_data) {
    UI *ui = (UI*)user_data;
    if (!ui || !event) return;
    (void)window;

    switch (event->type) {
        case WINDOW_EVENT_RESIZE:
            // Ignore resize events during drag — they're just position changes
            if (ui->dragging) break;
            if (event->width != ui->current_width || event->height != ui->current_height) {
                ui->current_width = event->width;
                ui->current_height = event->height;
                ui_calculate_layout(ui);
            }
            break;

        case WINDOW_EVENT_EXPOSE:
            // Window needs repaint (uncovered, mapped, etc.)
            ui->dirty = true;
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
