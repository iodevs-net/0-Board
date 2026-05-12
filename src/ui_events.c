// SPDX-License-Identifier: MIT — see LICENSE file

#include "ui_events.h"
#include "ui_internal.h"
#include "layout_engine.h"
#include "colors.h"
#include "debug.h"
#include "key_injector.h"
#include <X11/keysym.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

static bool rectangle_contains(Rectangle r, int x, int y) {
    return x >= r.x && x <= r.x + r.width && y >= r.y && y <= r.y + r.height;
}

/**
 * exec_async: Fork + exec a command non-blocking.
 * Returns PID of child, -1 on error.
 * Child runs in background, no zombie if we don't waitpid.
 * We set SIGCHLD to SIG_IGN in main() or handle properly.
 */
static pid_t exec_async(const char *path, char *const argv[]) {
    pid_t pid = fork();
    if (pid == 0) {
        // Child: exec the binary
        setsid(); // New session so signals don't propagate
        execvp(path, argv);
        // If exec fails, exit silently
        _exit(127);
    }
    return pid;
}

static bool is_on_edge(UI *ui, int wx, int wy) {
    int edge = 12;
    int top = ui->menu_visible ? MENU_BAR_HEIGHT : 0;
    return (wy >= top && wy < top + edge) ||
           (wy > ui->current_height - edge) ||
           (wx < edge) ||
           (wx > ui->current_width - edge);
}

static bool handle_menu_click(UI *ui, int mx, int my) {
    if (!ui->menu_visible || my >= MENU_BAR_HEIGHT) return false;
    if (rectangle_contains(ui->menu_btn_bounds[0], mx, my)) { ui_set_opacity(ui, ui->opacity - MENU_OPACITY_STEP); return true; }
    if (rectangle_contains(ui->menu_btn_bounds[1], mx, my)) { ui_set_opacity(ui, ui->opacity + MENU_OPACITY_STEP); return true; }
    if (rectangle_contains(ui->menu_btn_bounds[2], mx, my)) { ui_set_color_scheme(ui, (ui->color_scheme_index + 1) % NUM_COLOR_SCHEMES); return true; }
    if (rectangle_contains(ui->menu_btn_bounds[3], mx, my)) { ui->should_close = true; return true; }
    return false;
}

static bool handle_special_key(UI *ui, int key_index, const char *label) {
    (void)key_index;
    if (!label) return false;
    if (strcmp(label, "↑↓") == 0) {
        ui_toggle_dock_position(ui);
        return true;
    }
    return false;
}

static void handle_voice_key(UI *ui) {
    // Voice recording disabled — Cherry Trail SST audio DSP conflicts
    // with the touch controller when arecord captures from the default device.
    // The mic key still provides visual toggle feedback.
    // To re-enable: create /usr/local/bin/0-voice with proper ALSA device
    // parameters for the Intel SST DSP.
    ui->dirty = true;
}

static void handle_size_toggle(UI *ui, int key_index) {
    int wx, wy;
    x11_window_get_position(ui->window, &wx, &wy);
    Rectangle old_k = ui->key_bounds[key_index];
    double anchor_x = wx + old_k.x + old_k.width / 2.0;
    double anchor_y = wy + old_k.y + old_k.height / 2.0;
    int next_size = (ui->size_index + 1) % 3;
    ui_set_size_index(ui, next_size);
    Rectangle new_k = ui->key_bounds[key_index];
    int new_wx = (int)(anchor_x - (new_k.x + new_k.width / 2.0));
    int new_wy = (int)(anchor_y - (new_k.y + new_k.height / 2.0));
    if (ui->docked_top) new_wy = 0;
    ui_apply_geometry(ui, new_wx, new_wy);
}

void ui_handle_button_press(UI *ui, int wx, int wy, int rx, int ry, int button) {
    if (!ui) return;
    if (button == 2 || (button == 1 && is_on_edge(ui, wx, wy))) {
        drag_start(&ui->drag, ui->window, rx, ry);
        return;
    }
    if (button != 1) return;
    if (handle_menu_click(ui, wx, wy)) return;
    // Touchpad mode handling
    if (ui->touchpad_mode && handle_menu_click(ui, wx, wy)) return;

    if (ui->touchpad_mode) {
        /* Exit touchpad: red button */
        Touchpad *tp = &ui->touchpad;
        if (wx >= tp->exit_x && wx <= tp->exit_x + tp->exit_w &&
            wy >= tp->exit_y && wy <= tp->exit_y + tp->exit_h) {
            touchpad_warp_to_virtual(tp);
            ui->touchpad_mode = false;
            ui->dirty = true;
            return;
        }
        // Check L/R click buttons — warp to virtual pos first
        int btn;
        if (touchpad_is_button(tp, wx, wy, &btn)) {
            touchpad_warp_to_virtual(tp);
            engine_send_mouse_click(ui->engine, btn);
            return;
        }
        // Scroll zone — mark touching for motion handler
        if (touchpad_is_scroll(tp, wx, wy)) {
            tp->touching = true;
            tp->prev_y = wy;
            return;
        }
        // Regular touchpad area
        touchpad_down(tp, wx, wy);
        return;
    }

    for (int i = 0; i < ui->key_count; i++) {
        Rectangle *kb = &ui->key_bounds[i];
        if (wx < kb->x || wx > kb->x + kb->width || wy < kb->y || wy > kb->y + kb->height)
            continue;

        const char *label = keyboard_get_key_label(ui->keyboard, i);
        if (handle_special_key(ui, i, label)) break;

        KeySym sym = keyboard_get_keysym(ui->keyboard, i);
        if (sym == XK_Super_R) {
            KeyboardState st = keyboard_get_state(ui->keyboard);
            if (st.fn_state != KBD_MODIFIER_OFF) {
                if (ui->menu_visible) ui_hide_menu(ui); else ui_show_menu(ui);
            } else {
                handle_size_toggle(ui, i);
            }
            break;
        }
        if (sym == XK_Super_L) { keyboard_press_key(ui->keyboard, i); handle_voice_key(ui); break; }
        // ⦿ toggles touchpad mode
        if (label && strcmp(label, "⦿") == 0) {
            keyboard_press_key(ui->keyboard, i);
            ui->touchpad_mode = !ui->touchpad_mode;
            if (ui->touchpad_mode) {
                touchpad_init(&ui->touchpad);
                ui->touchpad.display = x11_window_get_display(ui->window);
                ui->touchpad.root = DefaultRootWindow(ui->touchpad.display);
                ui->touchpad.width = ui->current_width;
                ui->touchpad.height = ui->current_height;
            }
            ui->dirty = true;
            break;
        }

        keyboard_press_key(ui->keyboard, i);
        KeyDef *key = &keyboard_get_layout(ui->keyboard)->keys[i];
        if (!key_injector_is_modifier(key)) {
            int mods = keyboard_get_modifiers_for_keysym(ui->keyboard, sym);
            key_injector_send(ui->engine, x11_window_get_display(ui->window),
                              sym, mods, ui->config.key_event_delay_us);
            keyboard_notify_key_sent(ui->keyboard, i);
        }
        break;
    }
}

void ui_handle_button_release(UI *ui, int x, int y, int button) {
    if (ui->touchpad_mode) {
        if (ui->touchpad.touching) {
            int cx, cy;
            int btn = touchpad_up(&ui->touchpad, 0, 0, &cx, &cy);
            if (btn) engine_send_mouse_click(ui->engine, btn);
        }
        return;
    }
    if (button != 1) return;
    (void)x; (void)y;

    KeyboardState state = keyboard_get_state(ui->keyboard);
    if (state.pressed_key_index != -1) {
        keyboard_release_key(ui->keyboard, state.pressed_key_index);
    }
}

void ui_handle_motion(UI *ui, int rx, int ry) {
    if (!ui) return;
    drag_move(&ui->drag, ui->window, rx, ry);
}

void ui_event_callback(X11Window *window, WindowEvent *event, void *user_data) {
    UI *ui = (UI*)user_data;
    if (!ui || !event) return;
    (void)window;

    switch (event->type) {
        case WINDOW_EVENT_RESIZE:
            if (ui->drag.dragging) break;
            if (event->width != ui->current_width || event->height != ui->current_height) {
                ui->current_width = event->width;
                ui->current_height = event->height;
                ui_calculate_layout(ui);
            }
            break;

        case WINDOW_EVENT_EXPOSE:
            ui->dirty = true;
            break;

        case WINDOW_EVENT_MOTION:
            if (ui->touchpad_mode) {
                if (ui->touchpad.touching) {
                    // Check if in scroll zone
                    if (touchpad_is_scroll(&ui->touchpad, event->x, event->y)) {
                        int delta = touchpad_scroll_delta(&ui->touchpad, event->x, event->y);
                        if (delta != 0) {
                            touchpad_warp_to_virtual(&ui->touchpad);
                            engine_send_scroll(ui->engine, delta > 0 ? 5 : 4);
                        }
                    } else {
                        touchpad_motion(&ui->touchpad, event->x, event->y);
                    }
                }
            } else {
                ui_handle_motion(ui, event->root_x, event->root_y);
            }
            break;

        case WINDOW_EVENT_BUTTON_PRESS:
            ui_handle_button_press(ui, event->x, event->y,
                event->root_x, event->root_y, (int)event->button);
            break;

        case WINDOW_EVENT_BUTTON_RELEASE:
            drag_end(&ui->drag);
            ui_handle_button_release(ui, event->x, event->y, (int)event->button);
            break;

        case WINDOW_EVENT_CLOSE:
            ui->should_close = true;
            break;

        default:
            break;
    }
}
