/*
 * 0-Board Virtual Keyboard
 * Copyright (c) 2026 Leonardo Vergara <leonardovergaramarin@gmail.com>
 * Licensed under the MIT License.
 */
#include "ui_events.h"
#include "ui_internal.h"
#include "layout_engine.h"
#include "colors.h"
#include "debug.h"
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

void ui_handle_button_press(UI *ui, int wx, int wy, int rx, int ry, int button) {
    if (!ui) return;

    // Drag zone: any edge of the keyboard
    int edge = ui->current_height * DRAG_HANDLE_HEIGHT_RATIO;
    if (edge < 12) edge = 12;

    int keyboard_top = ui->menu_visible ? MENU_BAR_HEIGHT : 0;
    bool on_edge = false;

    if (wy >= keyboard_top && wy < keyboard_top + edge) on_edge = true;
    if (wy > ui->current_height - edge) on_edge = true;
    if (wx < edge) on_edge = true;
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

    // Menu bar buttons
    if (my < MENU_BAR_HEIGHT && ui->menu_visible) {
        if (rectangle_contains(ui->menu_btn_bounds[0], mx, my)) {
            ui_set_opacity(ui, ui->opacity - MENU_OPACITY_STEP);
        } else if (rectangle_contains(ui->menu_btn_bounds[1], mx, my)) {
            ui_set_opacity(ui, ui->opacity + MENU_OPACITY_STEP);
        } else if (rectangle_contains(ui->menu_btn_bounds[2], mx, my)) {
            ui_set_color_scheme(ui, (ui->color_scheme_index + 1) % NUM_COLOR_SCHEMES);
        } else if (rectangle_contains(ui->menu_btn_bounds[3], mx, my)) {
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
                    if (sym == XK_Super_R) {
                        // Size toggle with anchor
                        int wx, wy;
                        x11_window_get_position(ui->window, &wx, &wy);
                        Rectangle old_k = ui->key_bounds[i];

                        double anchor_x = wx + old_k.x + old_k.width / 2.0;
                        double anchor_y = wy + old_k.y + old_k.height / 2.0;

                        int next_size = (ui->size_index + 1) % 3;
                        ui_set_size_index(ui, next_size);

                        Rectangle new_k = ui->key_bounds[i];
                        int new_wx = (int)(anchor_x - (new_k.x + new_k.width / 2.0));
                        int new_wy = (int)(anchor_y - (new_k.y + new_k.height / 2.0));

                        if (ui->docked_top) new_wy = 0;

                        ui_apply_geometry(ui, new_wx, new_wy);
                    } else if (sym == XK_Super_L) {
                        // Microphone key: async voice recording toggle
                        const char *flag = ui->config.voice_recording_flag;
                        if (flag && flag[0] && access(flag, F_OK) == 0) {
                            // Recording active → stop
                            exec_async("/usr/bin/pkill", (char *[]) {
                                "pkill", "-TERM", "arecord", NULL
                            });
                            // Also try killall
                            exec_async("/usr/bin/killall", (char *[]) {
                                "killall", "-q", "arecord", NULL
                            });
                        } else {
                            // Start recording
                            const char *script = ui->config.voice_script_path;
                            if (script && script[0]) {
                                exec_async(script, (char *[]) {
                                    (char *)script, NULL
                                });
                            }
                        }
                        ui->dirty = true;
                    } else {
                        keyboard_press_key(ui->keyboard, i);

                        KeyDef *key = &keyboard_get_layout(ui->keyboard)->keys[i];
                        bool is_modifier = (key->flags & (KEYFLAG_SHIFT | KEYFLAG_CTRL | KEYFLAG_ALT | KEYFLAG_META)) ||
                                         (key->normal == XK_Caps_Lock);

                        if (!is_modifier) {
                            // Use keysym-aware modifiers (handles Caps Lock → Shift for letters)
                            int mods = keyboard_get_modifiers_for_keysym(ui->keyboard, sym);
                            engine_send_key_ex(ui->engine, sym, true, mods);
                            XFlush(x11_window_get_display(ui->window));

                            // Event delay: small enough to not block UI, large enough for key repeat
                            if (ui->config.key_event_delay_us > 0) {
                                usleep(ui->config.key_event_delay_us);
                            }

                            engine_send_key_ex(ui->engine, sym, false, mods);
                            engine_flush(ui->engine);

                            keyboard_notify_key_sent(ui->keyboard, i);
                        }
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
}

void ui_event_callback(X11Window *window, WindowEvent *event, void *user_data) {
    UI *ui = (UI*)user_data;
    if (!ui || !event) return;
    (void)window;

    switch (event->type) {
        case WINDOW_EVENT_RESIZE:
            if (ui->dragging) break;
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
