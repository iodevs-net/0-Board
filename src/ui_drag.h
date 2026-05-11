/*
 * 0-Board Virtual Keyboard
 * Copyright (c) 2026 Leonardo Vergara <leonardovergaramarin@gmail.com>
 * Licensed under the MIT License.
 */
#ifndef UI_DRAG_H
#define UI_DRAG_H

#include <stdbool.h>
#include "x11_window.h"

// Drag state: tracks active window-drag session
typedef struct {
    bool dragging;
    int offset_x;
    int offset_y;
} DragState;

// Initialize drag state (all fields to zero / false)
void drag_init(DragState *state);

// Start a drag session: record the offset between pointer and window origin
void drag_start(DragState *state, X11Window *window, int root_x, int root_y);

// Continue an active drag: move the window to follow the pointer
void drag_move(DragState *state, X11Window *window, int root_x, int root_y);

// End the drag session
void drag_end(DragState *state);

#endif // UI_DRAG_H
