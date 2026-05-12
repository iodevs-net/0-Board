// SPDX-License-Identifier: MIT — see LICENSE file
//
// X11 event processing and error recovery.
// Moved from x11_window.c during hardening refactor.

#include "x11_window_internal.h"
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>

// Window registry for error handler lookup
X11Window *g_x11_windows[MAX_X11_WINDOWS] = {0};
int g_x11_window_count = 0;

static int x11_error_handler(Display *dpy, XErrorEvent *err) {
    (void)err;
    for (int i = 0; i < g_x11_window_count; i++) {
        if (g_x11_windows[i] && g_x11_windows[i]->display == dpy) {
            g_x11_windows[i]->fatal_error = true;
            break;
        }
    }
    return 0;
}

void x11_register_window(X11Window *win) {
    if (!win) return;
    if (g_x11_window_count == 0) {
        // First window — install error handler
        XSetErrorHandler(x11_error_handler);
    }
    // Check for duplicates
    for (int i = 0; i < g_x11_window_count; i++) {
        if (g_x11_windows[i] == win) return;
    }
    if (g_x11_window_count < MAX_X11_WINDOWS) {
        g_x11_windows[g_x11_window_count++] = win;
    }
}

void x11_unregister_window(X11Window *win) {
    if (!win) return;
    for (int i = 0; i < g_x11_window_count; i++) {
        if (g_x11_windows[i] == win) {
            g_x11_windows[i] = g_x11_windows[--g_x11_window_count];
            break;
        }
    }
}

static void process_xevent(X11Window *w, XEvent *xev) {
    if (!w || !xev || !w->event_callback) return;
    WindowEvent event = {0};
    switch (xev->type) {
        case Expose:
            event.type = WINDOW_EVENT_EXPOSE;
            event.x = xev->xexpose.x; event.y = xev->xexpose.y;
            event.width = xev->xexpose.width; event.height = xev->xexpose.height;
            break;
        case ButtonPress:
            event.type = WINDOW_EVENT_BUTTON_PRESS;
            event.x = xev->xbutton.x; event.y = xev->xbutton.y;
            event.root_x = xev->xbutton.x_root; event.root_y = xev->xbutton.y_root;
            event.button = (MouseButton)xev->xbutton.button;
            event.state = xev->xbutton.state;
            break;
        case ButtonRelease:
            event.type = WINDOW_EVENT_BUTTON_RELEASE;
            event.x = xev->xbutton.x; event.y = xev->xbutton.y;
            event.root_x = xev->xbutton.x_root; event.root_y = xev->xbutton.y_root;
            event.button = (MouseButton)xev->xbutton.button;
            event.state = xev->xbutton.state;
            break;
        case MotionNotify:
            event.type = WINDOW_EVENT_MOTION;
            event.x = xev->xmotion.x; event.y = xev->xmotion.y;
            event.root_x = xev->xmotion.x_root; event.root_y = xev->xmotion.y_root;
            event.state = xev->xmotion.state;
            break;
        case ClientMessage:
            if (w->wm_delete_window != None &&
                (Atom)xev->xclient.data.l[0] == w->wm_delete_window) {
                event.type = WINDOW_EVENT_CLOSE;
            }
            break;
        case ConfigureNotify:
            event.type = WINDOW_EVENT_RESIZE;
            event.width = xev->xconfigure.width;
            event.height = xev->xconfigure.height;
            break;
        default:
            return;
    }
    w->event_callback(w, &event, w->event_user_data);
}

bool x11_window_process_events(X11Window *w) {
    if (!w || !w->display || w->fatal_error) return false;
    XEvent xev;
    bool had = false;
    int pending = XEventsQueued(w->display, QueuedAfterReading);
    // Limit per-frame events to prevent starvation on X11 flood
    int max_events = 64;
    while (pending > 0 && max_events > 0) {
        XNextEvent(w->display, &xev);
        process_xevent(w, &xev);
        had = true;
        pending--;
        max_events--;
    }
    return had;
}

bool x11_window_wait_event(X11Window *w, int timeout_ms) {
    if (!w || !w->display || w->fatal_error) return false;

    // Flush any pending requests and check if we already have events
    if (XEventsQueued(w->display, QueuedAfterFlush) > 0) {
        return x11_window_process_events(w);
    }

    if (timeout_ms > 0) {
        fd_set fds; FD_ZERO(&fds);
        int fd = ConnectionNumber(w->display);
        FD_SET(fd, &fds);
        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        int ret = select(fd + 1, &fds, NULL, NULL, &tv);
        if (ret < 0 && errno != EINTR) {
            w->fatal_error = true;
            return false;
        }
        if (ret > 0) return x11_window_process_events(w);
        return false;
    }

    // Blocking wait (capped at 500ms to check shutdown)
    fd_set fds; FD_ZERO(&fds);
    int fd = ConnectionNumber(w->display);
    FD_SET(fd, &fds);
    struct timeval tv = {0, 500000}; // 500ms max
    int ret = select(fd + 1, &fds, NULL, NULL, &tv);
    if (ret > 0) return x11_window_process_events(w);
    return false;
}
