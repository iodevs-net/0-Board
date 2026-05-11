// SPDX-License-Identifier: MIT — see LICENSE file

#include "ui_drag.h"

void drag_init(DragState *state) {
    if (!state) return;
    state->dragging = false;
    state->offset_x = 0;
    state->offset_y = 0;
}

void drag_start(DragState *state, X11Window *window, int root_x, int root_y) {
    if (!state || !window) return;
    int win_x, win_y;
    x11_window_get_position(window, &win_x, &win_y);
    state->offset_x = root_x - win_x;
    state->offset_y = root_y - win_y;
    state->dragging = true;
}

void drag_move(DragState *state, X11Window *window, int root_x, int root_y) {
    if (!state || !state->dragging || !window) return;
    x11_window_move(window, root_x - state->offset_x, root_y - state->offset_y);
}

void drag_end(DragState *state) {
    if (!state) return;
    state->dragging = false;
}
