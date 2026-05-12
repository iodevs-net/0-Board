// SPDX-License-Identifier: MIT — see LICENSE file

#include "touchpad.h"
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

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
    tp->touchscreen_id = -1;
}

/* Find touchscreen xinput device ID by scanning names.
 * We look for devices with "Touchscreen" or "touch" in the name.
 * Returns device ID or -1 if not found. */
static int find_touchscreen_id(void) {
    /* Parse xinput list: find slave pointer that is NOT Virtual/XTEST/Stylus/eraser.
     * On the HP x2 210 G1 the touchscreen is "SYNA7508:00 06CB:1613" (no "touch" in name). */
    FILE *fp = popen("xinput list 2>/dev/null", "r");
    if (!fp) return -1;
    char line[512];
    int found_id = -1;
    while (fgets(line, sizeof(line), fp)) {
        /* Must be a slave pointer */
        if (!strstr(line, "slave  pointer")) continue;
        /* Skip known non-touchscreen devices */
        char lower[512];
        int i;
        for (i = 0; line[i] && i < 511; i++)
            lower[i] = (line[i] >= 'A' && line[i] <= 'Z') ? line[i] + 32 : line[i];
        lower[i] = '\0';
        if (strstr(lower, "virtual") || strstr(lower, "xtest") ||
            strstr(lower, "stylus") || strstr(lower, "eraser") ||
            strstr(lower, "touchpad") || strstr(lower, "mouse") ||
            strstr(lower, "trackpoint"))
            continue;
        /* This is likely the touchscreen — extract id=N */
        char *id_str = strstr(line, "id=");
        if (id_str) {
            found_id = atoi(id_str + 3);
            if (found_id > 0) break;
        }
    }
    pclose(fp);
    return found_id;
}

void touchpad_grab_touchscreen(Touchpad *tp) {
    if (tp->touchscreen_id < 0)
        tp->touchscreen_id = find_touchscreen_id();
    if (tp->touchscreen_id > 0) {
        char cmd[64];
        snprintf(cmd, sizeof(cmd), "xinput disable %d", tp->touchscreen_id);
        if (system(cmd)) { /* best effort */ }
    }
}

void touchpad_release_touchscreen(Touchpad *tp) {
    if (tp->touchscreen_id > 0) {
        char cmd[64];
        snprintf(cmd, sizeof(cmd), "xinput enable %d", tp->touchscreen_id);
        if (system(cmd)) { /* best effort */ }
    }
}

void touchpad_down(Touchpad *tp, int touch_x, int touch_y) {
    tp->touching = true;

    if (tp->virt_x == 0 && tp->virt_y == 0) {
        /* Primer toque: leer posicion real del cursor */
        Window root_ret, child_ret;
        int root_x, root_y, win_x, win_y;
        unsigned int mask;
        if (tp->display) {
            XQueryPointer(tp->display, tp->root, &root_ret, &child_ret,
                          &root_x, &root_y, &win_x, &win_y, &mask);
            tp->virt_x = root_x;
            tp->virt_y = root_y;
        }
    }
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

    tp->virt_x += dx * tp->acceleration;
    tp->virt_y += dy * tp->acceleration;

    /* Move system pointer relatively */
    XWarpPointer(tp->display, None, None, 0, 0, 0, 0,
                 dx * tp->acceleration, dy * tp->acceleration);
    XFlush(tp->display);

    tp->prev_x = touch_x;
    tp->prev_y = touch_y;
    return true;
}

int touchpad_up(Touchpad *tp, int touch_x, int touch_y, int *click_at_x, int *click_at_y) {
    (void)touch_x; (void)touch_y; (void)click_at_x; (void)click_at_y;
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
