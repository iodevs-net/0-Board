// SPDX-License-Identifier: MIT — see LICENSE file

#include "touchpad.h"
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

static unsigned long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void touchpad_init(Touchpad *tp) {
    memset(tp, 0, sizeof(*tp));
    tp->acceleration = 2;
    tp->long_press_ms = 500;
    tp->scroll_sensitivity = 20;
}

void touchpad_down(Touchpad *tp, int touch_x, int touch_y) {
    tp->touching = true;

    /* Leer posicion real del cursor para arrancar desde ahi */
    Window root_ret, child_ret;
    int root_x, root_y, win_x, win_y;
    unsigned int mask;
    if (tp->display) {
        XQueryPointer(tp->display, tp->root, &root_ret, &child_ret,
                      &root_x, &root_y, &win_x, &win_y, &mask);
        tp->start_x = root_x;
        tp->start_y = root_y;
    }
    tp->virt_x = tp->start_x;
    tp->virt_y = tp->start_y;
    tp->prev_x = touch_x;
    tp->prev_y = touch_y;
    tp->touch_start_x = touch_x;
    tp->touch_start_y = touch_y;
    tp->touch_start_time = now_ms();
    tp->moved = false;
}

bool touchpad_motion(Touchpad *tp, int touch_x, int touch_y) {
    if (!tp->touching || !tp->display) return false;

    int dx = touch_x - tp->prev_x;
    int dy = touch_y - tp->prev_y;

    if (dx != 0 || dy != 0) {
        int dist = dx*dx + dy*dy;
        if (dist > 10) tp->moved = true;
    }

    if (dx == 0 && dy == 0) return false;

    // Update virtual pointer position
    tp->virt_x += dx * tp->acceleration;
    tp->virt_y += dy * tp->acceleration;

    XWarpPointer(tp->display, None, tp->root, 0, 0, 0, 0,
                 tp->virt_x, tp->virt_y);
    XFlush(tp->display);

    tp->prev_x = touch_x;
    tp->prev_y = touch_y;
    return true;
}

int touchpad_up(Touchpad *tp, int touch_x, int touch_y, int *click_at_x, int *click_at_y) {
    if (!tp->touching) return 0;
    tp->touching = false;
    unsigned long elapsed = now_ms() - tp->touch_start_time;
    if (tp->moved) return 0;
    if (elapsed >= (unsigned long)tp->long_press_ms) return 3;
    return 1;
}

bool touchpad_is_scroll(Touchpad *tp, int touch_x, int touch_y) {
    if (touch_x < tp->scroll_bar_x || touch_y < 0) return false;
    int bottom = tp->height - tp->btn_h - tp->scroll_bar_x;
    return touch_y < bottom;
}

int touchpad_scroll_delta(Touchpad *tp, int touch_x, int touch_y) {
    if (!tp->touching) return 0;
    int dy = touch_y - tp->prev_y;
    tp->prev_y = touch_y;
    if (abs(dy) < tp->scroll_sensitivity) return 0;
    return dy > 0 ? 1 : -1;
}

bool touchpad_is_button(Touchpad *tp, int touch_x, int touch_y, int *button) {
    if (touch_y < tp->btn_y || touch_y > tp->btn_y + tp->btn_h) return false;
    if (touch_x >= tp->btn_left_x && touch_x < tp->btn_left_x + tp->btn_w) {
        *button = 1; return true;
    }
    if (touch_x >= tp->btn_right_x && touch_x < tp->btn_right_x + tp->btn_w) {
        *button = 3; return true;
    }
    return false;
}
