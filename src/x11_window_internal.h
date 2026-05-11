// SPDX-License-Identifier: MIT — see LICENSE file
#ifndef X11_WINDOW_INTERNAL_H
#define X11_WINDOW_INTERNAL_H

#include <X11/Xlib.h>
#include <stdbool.h>
#include "x11_window.h"

struct X11Window {
    Display *display;
    Window window;
    Visual *visual;
    int depth;
    int screen;
    int width, height;
    GC gc;
    Atom wm_delete_window;
    WindowEventCallback event_callback;
    void *event_user_data;
    bool owns_display;
    bool fatal_error;
};

#define MAX_X11_WINDOWS 16

extern X11Window *g_x11_windows[MAX_X11_WINDOWS];
extern int g_x11_window_count;

void x11_register_window(X11Window *win);
void x11_unregister_window(X11Window *win);

#endif // X11_WINDOW_INTERNAL_H
