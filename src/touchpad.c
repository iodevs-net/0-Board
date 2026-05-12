// SPDX-License-Identifier: MIT — see LICENSE file

#include "touchpad.h"
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

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

void touchpad_update_virtual_to_real(Touchpad *tp) {
    if (!tp->display) return;
    Window root_ret, child_ret;
    int root_x, root_y, win_x, win_y;
    unsigned int mask;
    if (XQueryPointer(tp->display, tp->root, &root_ret, &child_ret,
                  &root_x, &root_y, &win_x, &win_y, &mask)) {
        tp->virt_x = root_x;
        tp->virt_y = root_y;
    }
}

void touchpad_down(Touchpad *tp, int touch_x, int touch_y) {
    tp->touching = true;

    tp->prev_x = touch_x;
    tp->prev_y = touch_y;
    tp->touch_start_x = touch_x;
    tp->touch_start_y = touch_y;
    tp->touch_start_time = now_ms();
    tp->moved = false;
}

bool touchpad_motion(Touchpad *tp, int touch_x, int touch_y) {
    if (!tp->touching) return false;

    int dx = touch_x - tp->prev_x;
    int dy = touch_y - tp->prev_y;

    if (dx == 0 && dy == 0) return false;

    if (dx*dx + dy*dy > 10) tp->moved = true;

    /* Update virtual position only — do NOT warp the system pointer.
     * The touchscreen would fight us with absolute coordinates.
     * We warp only on finger-up or button click. */
    tp->virt_x += dx * tp->acceleration;
    tp->virt_y += dy * tp->acceleration;

    tp->prev_x = touch_x;
    tp->prev_y = touch_y;
    return true;
}

void touchpad_warp_to_virtual(Touchpad *tp) {
    if (!tp->display) return;
    XWarpPointer(tp->display, None, tp->root, 0, 0, 0, 0,
                 tp->virt_x, tp->virt_y);
    XFlush(tp->display);
}

void touchpad_click_at_virtual(Touchpad *tp, int button) {
    if (!tp->display) return;
    /* Warp to virtual position */
    XWarpPointer(tp->display, None, tp->root, 0, 0, 0, 0,
                 tp->virt_x, tp->virt_y);
    XFlush(tp->display);
    
    /* We must have a small delay between Press and Release, otherwise
     * many GUI toolkits will ignore the 0-duration click. */
    XTestFakeButtonEvent(tp->display, button, True, 0);
    XFlush(tp->display);
    usleep(20000); /* 20ms */
    
    /* Warp again just in case the touchscreen moved the pointer back during the sleep */
    XWarpPointer(tp->display, None, tp->root, 0, 0, 0, 0,
                 tp->virt_x, tp->virt_y);
    XTestFakeButtonEvent(tp->display, button, False, 0);
    XFlush(tp->display);
}

void touchpad_scroll_at_virtual(Touchpad *tp, int button) {
    if (!tp->display) return;
    /* Warp + scroll as atomic burst */
    XWarpPointer(tp->display, None, tp->root, 0, 0, 0, 0,
                 tp->virt_x, tp->virt_y);
    XTestFakeButtonEvent(tp->display, button, True, 0);
    XTestFakeButtonEvent(tp->display, button, False, 0);
    XFlush(tp->display);
}

int touchpad_up(Touchpad *tp, int touch_x, int touch_y, int *click_at_x, int *click_at_y) {
    (void)touch_x; (void)touch_y; (void)click_at_x; (void)click_at_y;
    if (!tp->touching) return 0;
    tp->touching = false;

    unsigned long elapsed = now_ms() - tp->touch_start_time;
    if (tp->moved) {
        /* Drag completed — warp to final virtual position */
        touchpad_warp_to_virtual(tp);
        return 0;
    }
    /* Tap or long press — atomic warp+click */
    int button = (elapsed >= (unsigned long)tp->long_press_ms) ? 3 : 1;
    touchpad_click_at_virtual(tp, button);
    return 0;  /* click already sent */
}

bool touchpad_is_scroll(Touchpad *tp, int touch_x, int touch_y) {
    if (touch_x < tp->scroll_bar_x || touch_y < 0) return false;
    int bottom = tp->height - tp->btn_h - tp->scroll_bar_x;
    return touch_y < bottom;
}

int touchpad_scroll_delta(Touchpad *tp, int touch_x, int touch_y) {
    (void)touch_x;
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
