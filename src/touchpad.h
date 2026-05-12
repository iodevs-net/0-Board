// SPDX-License-Identifier: MIT — see LICENSE file

#ifndef TOUCHPAD_H
#define TOUCHPAD_H

#include <X11/Xlib.h>
#include <stdbool.h>

typedef struct {
    // State
    bool active;
    bool touching;
    int prev_x, prev_y;     // Previous touch position for delta tracking
    int virt_x, virt_y;     // Virtual pointer position (relative from start)
    int start_x, start_y;   // Saved pointer position (warp back here)
    int touch_start_x, touch_start_y;
    unsigned long touch_start_time;  // ms for long-press detection
    bool moved;              // Did finger move significantly since touch down

    // Config
    int acceleration;       // Pointer speed multiplier (default 2)
    int scroll_sensitivity; // Scroll wheel events per pixel (default 1 per 20px)
    int long_press_ms;      // ms to trigger right-click (default 500)

    // UI metrics (set by ui_render_touchpad)
    int width, height;
    int scroll_bar_x;       // X position of scroll bar
    int btn_y;                 // Y position of buttons
    int btn_left_x, btn_left_y, btn_w, btn_h;   // Left click button coords
    int btn_right_x, btn_right_y;                // Right click button
    int exit_x, exit_y, exit_w, exit_h;
    int warp_skip;              // contador: ignorar N motion events tras XWarpPointer
    int touchscreen_id;         // xinput device ID of touchscreen (-1 = unknown)

    // Display (set by user)
    Display *display;
    Window root;
} Touchpad;

void touchpad_init(Touchpad *tp);
void touchpad_grab_touchscreen(Touchpad *tp);
void touchpad_release_touchscreen(Touchpad *tp);
void touchpad_down(Touchpad *tp, int touch_x, int touch_y);
bool touchpad_motion(Touchpad *tp, int touch_x, int touch_y);
int touchpad_up(Touchpad *tp, int touch_x, int touch_y, int *click_at_x, int *click_at_y);
bool touchpad_is_scroll(Touchpad *tp, int touch_x, int touch_y);
int touchpad_scroll_delta(Touchpad *tp, int touch_x, int touch_y);
bool touchpad_is_button(Touchpad *tp, int touch_x, int touch_y, int *button);

#endif
