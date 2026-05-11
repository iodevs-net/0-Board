// SPDX-License-Identifier: MIT — see LICENSE file

#include "x11_window.h"
#include "x11_window_internal.h"
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "constants.h"


static Atom get_atom(X11Window *win, const char *name) {
    if (!win || !win->display) return None;
    return XInternAtom(win->display, name, False);
}

X11Window* x11_window_create(WindowConfig *config) {
    if (!config) { fprintf(stderr, "x11_window_create: config NULL\n"); return NULL; }

    X11Window *win = calloc(1, sizeof(X11Window));
    if (!win) { fprintf(stderr, "x11_window_create: OOM\n"); return NULL; }

    win->display = XOpenDisplay(NULL);
    if (!win->display) { fprintf(stderr, "x11_window_create: no display\n"); free(win); return NULL; }
    win->owns_display = true;
    win->screen = DefaultScreen(win->display);


    XVisualInfo vi;
    int found = XMatchVisualInfo(win->display, win->screen, 32, TrueColor, &vi);
    if (!found) {
        vi.visual = DefaultVisual(win->display, win->screen);
        vi.depth = DefaultDepth(win->display, win->screen);
    }
    win->visual = vi.visual;
    win->depth = vi.depth;

    Window root = DefaultRootWindow(win->display);
    XSetWindowAttributes wa = {0};
    wa.colormap = XCreateColormap(win->display, root, win->visual, AllocNone);
    wa.border_pixel = 0;
    wa.background_pixel = 0;
    wa.override_redirect = config->override_redirect ? True : False;

    unsigned long mask = CWColormap | CWBorderPixel | CWBackPixel;
    if (config->override_redirect) mask |= CWOverrideRedirect;

    int width = config->width;
    int height = config->height;
    if (width <= 0) {
        int sw = DisplayWidth(win->display, win->screen);
        double ratios[] = {SCREEN_WIDTH_RATIO_SMALL, SCREEN_WIDTH_RATIO_MEDIUM, SCREEN_WIDTH_RATIO_LARGE};
        int idx = (config->initial_size_index >= 0 && config->initial_size_index < 3) ? config->initial_size_index : 1;
        width = sw * ratios[idx];
        height = width * KEYBOARD_HEIGHT_RATIO;
    }

    int x = config->x >= 0 ? config->x : 100;
    int y = config->y >= 0 ? config->y : 100;

    win->window = XCreateWindow(win->display, root, x, y, width, height, 0,
                                win->depth, InputOutput, win->visual, mask, &wa);
    win->width = width;
    win->height = height;

    if (!win->window) {
        fprintf(stderr, "x11_window_create: XCreateWindow failed\n");
        XCloseDisplay(win->display);
        free(win);
        return NULL;
    }

    Atom type_atom = get_atom(win, "_NET_WM_WINDOW_TYPE");
    Atom dock_atom = get_atom(win, "_NET_WM_WINDOW_TYPE_DOCK");
    if (type_atom != None && dock_atom != None)
        XChangeProperty(win->display, win->window, type_atom, XA_ATOM, 32,
                        PropModeReplace, (unsigned char*)&dock_atom, 1);

    Atom wm_state = get_atom(win, "_NET_WM_STATE");
    Atom wm_above = get_atom(win, "_NET_WM_STATE_ABOVE");
    if (wm_state != None && wm_above != None)
        XChangeProperty(win->display, win->window, wm_state, XA_ATOM, 32,
                        PropModeReplace, (unsigned char*)&wm_above, 1);

    if (config->borderless) {
        Atom mwm = get_atom(win, "_MOTIF_WM_HINTS");
        if (mwm != None) {
            unsigned long hints[5] = {2, 0, 0, 0, 0};
            XChangeProperty(win->display, win->window, mwm, mwm, 32,
                            PropModeReplace, (unsigned char*)hints, 5);
        }
    }

    if (config->title) XStoreName(win->display, win->window, config->title);
    // Set WM_CLASS so window managers (twin, etc.) can identify and apply rules
    {
        XClassHint hint;
        hint.res_name = (char*)"0-board";
        hint.res_class = (char*)"0-board";
        XSetClassHint(win->display, win->window, &hint);
    }

    XWMHints *wmh = XAllocWMHints();
    if (wmh) { wmh->flags = InputHint; wmh->input = False; XSetWMHints(win->display, win->window, wmh); XFree(wmh); }

    win->gc = XCreateGC(win->display, win->window, 0, NULL);

    XSelectInput(win->display, win->window,
        ExposureMask | ButtonPressMask | ButtonReleaseMask | Button1MotionMask | StructureNotifyMask);

    win->wm_delete_window = get_atom(win, "WM_DELETE_WINDOW");
    if (win->wm_delete_window != None)
        XSetWMProtocols(win->display, win->window, &win->wm_delete_window, 1);

    if (config->opacity < 1.0) {
        Atom oa = get_atom(win, "_NET_WM_WINDOW_OPACITY");
        if (oa != None) {
            unsigned long op = (unsigned long)(0xFFFFFFFFUL * config->opacity);
            XChangeProperty(win->display, win->window, oa, XA_CARDINAL, 32,
                            PropModeReplace, (unsigned char*)&op, 1);
        }
    }

    win->event_callback = NULL;
    win->event_user_data = NULL;
    x11_register_window(win);
    return win;
}

void x11_window_set_event_callback(X11Window *w, WindowEventCallback cb, void *ud) {
    if (!w) return;
    w->event_callback = cb;
    w->event_user_data = ud;
}

Display* x11_window_get_display(X11Window *w) { return w ? w->display : NULL; }
Window x11_window_get_id(X11Window *w) { return w ? w->window : 0; }
Visual* x11_window_get_visual(X11Window *w) { return w ? w->visual : NULL; }
int x11_window_get_depth(X11Window *w) { return w ? w->depth : 0; }
int x11_window_get_width(X11Window *w) { return w ? w->width : 0; }
int x11_window_get_height(X11Window *w) { return w ? w->height : 0; }
bool x11_window_has_error(X11Window *w) { return w ? w->fatal_error : true; }

Pixmap x11_window_create_pixmap(X11Window *w, int width, int height) {
    if (!w || !w->display || width <= 0 || height <= 0) return 0;
    return XCreatePixmap(w->display, w->window, width, height, w->depth);
}

void x11_window_copy_area(X11Window *w, Pixmap src, int sx, int sy, int sw, int sh, int dx, int dy) {
    if (!w || !w->display || !src) return;
    XCopyArea(w->display, src, w->window, w->gc, sx, sy, sw, sh, dx, dy);
}

void x11_window_move(X11Window *w, int x, int y) {
    if (!w || !w->display) return;
    XMoveWindow(w->display, w->window, x, y);
}

void x11_window_get_position(X11Window *w, int *x, int *y) {
    if (x) *x = 0;
    if (y) *y = 0;
    if (!w || !w->display) return;
    Window child; int rx, ry;
    XTranslateCoordinates(w->display, w->window, DefaultRootWindow(w->display), 0, 0, &rx, &ry, &child);
    if (x) *x = rx;
    if (y) *y = ry;
}

void x11_window_resize(X11Window *w, int width, int height) {
    if (!w || !w->display || width <= 0 || height <= 0) return;
    XResizeWindow(w->display, w->window, width, height);
    w->width = width; w->height = height;
}

void x11_window_move_resize(X11Window *w, int x, int y, int width, int height) {
    if (!w || !w->display || width <= 0 || height <= 0) return;
    XMoveResizeWindow(w->display, w->window, x, y, width, height);
    w->width = width; w->height = height;
}

void x11_window_get_size(X11Window *w, int *width, int *height) {
    if (!w) return;
    if (width) *width = w->width;
    if (height) *height = w->height;
}

void x11_window_set_title(X11Window *w, const char *title) {
    if (!w || !w->display || !title) return;
    XStoreName(w->display, w->window, title);
}

void x11_window_set_opacity(X11Window *w, double opacity) {
    if (!w) return;
    Atom oa = get_atom(w, "_NET_WM_WINDOW_OPACITY");
    if (oa != None) {
        unsigned long v = (unsigned long)(opacity * 0xFFFFFFFF);
        XChangeProperty(w->display, w->window, oa, XA_CARDINAL, 32,
                        PropModeReplace, (unsigned char*)&v, 1);
    }
}

void x11_window_set_always_on_top(X11Window *w, bool on_top) {
    if (!w) return;
    Atom wm_state = get_atom(w, "_NET_WM_STATE");
    Atom wm_above = get_atom(w, "_NET_WM_STATE_ABOVE");
    if (wm_state == None || wm_above == None) return;
    XEvent xev = {0};
    xev.type = ClientMessage;
    xev.xclient.window = w->window;
    xev.xclient.message_type = wm_state;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = on_top ? 1 : 0;
    xev.xclient.data.l[1] = wm_above;
    XSendEvent(w->display, DefaultRootWindow(w->display), False,
               SubstructureRedirectMask | SubstructureNotifyMask, &xev);
}

void x11_window_show(X11Window *w) {
    if (!w || !w->display) return;
    XMapWindow(w->display, w->window);
    XFlush(w->display);
}

void x11_window_hide(X11Window *w) {
    if (!w || !w->display) return;
    XUnmapWindow(w->display, w->window);
    XFlush(w->display);
}


void x11_window_close(X11Window *w) {
    if (!w || !w->display || !w->window) return;
    XDestroyWindow(w->display, w->window);
    w->window = 0;
}

void x11_window_destroy(X11Window *w) {
    if (!w) return;
    x11_unregister_window(w);
    if (w->gc && w->display) XFreeGC(w->display, w->gc);
    if (w->window && w->display) XDestroyWindow(w->display, w->window);
    if (w->display && w->owns_display) XCloseDisplay(w->display);
    free(w);
}
