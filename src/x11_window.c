#include "x11_window.h"
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "constants.h"
#include <sys/select.h>
#include <sys/time.h>

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
};

static Atom get_atom(X11Window *win, const char *name) {
    return XInternAtom(win->display, name, False);
}

X11Window* x11_window_create(WindowConfig *config) {
    if (!config) {
        fprintf(stderr, "x11_window_create: config cannot be NULL\n");
        return NULL;
    }
    
    X11Window *win = malloc(sizeof(X11Window));
    if (!win) {
        fprintf(stderr, "x11_window_create: failed to allocate memory\n");
        return NULL;
    }
    
    memset(win, 0, sizeof(X11Window));
    
    // Open display
    win->display = XOpenDisplay(NULL);
    if (!win->display) {
        fprintf(stderr, "x11_window_create: failed to open X11 display\n");
        free(win);
        return NULL;
    }
    win->owns_display = true;
    
    // Get visual
    XVisualInfo vi;
    if (!XMatchVisualInfo(win->display, DefaultScreen(win->display), 32, TrueColor, &vi)) {
        vi.visual = DefaultVisual(win->display, DefaultScreen(win->display));
        vi.depth = DefaultDepth(win->display, DefaultScreen(win->display));
    }
    win->visual = vi.visual;
    win->depth = vi.depth;
    
    // Create window
    Window root = DefaultRootWindow(win->display);
    
    XSetWindowAttributes wa;
    wa.colormap = XCreateColormap(win->display, root, win->visual, AllocNone);
    wa.border_pixel = 0;
    wa.background_pixel = 0;
    wa.override_redirect = config->override_redirect ? True : False;
    
    unsigned long mask = CWColormap | CWBorderPixel | CWBackPixel;
    if (config->override_redirect) {
        mask |= CWOverrideRedirect;
    }
    
    // Auto-size or use config
    int width = config->width;
    int height = config->height;
    if (width <= 0) {
        int screen_w = DisplayWidth(win->display, win->screen);
        double ratios[] = {SCREEN_WIDTH_RATIO_SMALL, SCREEN_WIDTH_RATIO_MEDIUM, SCREEN_WIDTH_RATIO_LARGE};
        int idx = (config->initial_size_index >= 0 && config->initial_size_index < 3) ? config->initial_size_index : 1;
        width = screen_w * ratios[idx];
        height = width * KEYBOARD_HEIGHT_RATIO;
    }

    // Use default position if not specified
    int x = config->x >= 0 ? config->x : 100;
    int y = config->y >= 0 ? config->y : 100;
    
    win->window = XCreateWindow(win->display, root, x, y, 
                                width, height, 
                                0, win->depth, InputOutput, win->visual, 
                                mask, &wa);
    
    win->width = width;
    win->height = height;
    
    if (!win->window) {
        fprintf(stderr, "x11_window_create: failed to create window\n");
        XCloseDisplay(win->display);
        free(win);
        return NULL;
    }
    
    // Remove decorations if borderless
    // Forzar modo DOCK (Siempre al frente y reservando espacio si el WM lo soporta)
    Atom type_atom = get_atom(win, "_NET_WM_WINDOW_TYPE");
    Atom dock_atom = get_atom(win, "_NET_WM_WINDOW_TYPE_DOCK");
    if (type_atom != None && dock_atom != None) {
        XChangeProperty(win->display, win->window, type_atom, XA_ATOM, 32, PropModeReplace, (unsigned char*)&dock_atom, 1);
    }

    // Always on Top adicional para WMs que ignoran el DOCK
    Atom wm_state = get_atom(win, "_NET_WM_STATE");
    Atom wm_above = get_atom(win, "_NET_WM_STATE_ABOVE");
    if (wm_state != None && wm_above != None) {
        XChangeProperty(win->display, win->window, wm_state, XA_ATOM, 32, PropModeReplace, (unsigned char*)&wm_above, 1);
    }
    
    if (config->borderless) {
        Atom mwm_hints = get_atom(win, "_MOTIF_WM_HINTS");
        if (mwm_hints != None) {
            struct {
                unsigned long flags;
                unsigned long functions;
                unsigned long decorations;
                long input_mode;
                unsigned long status;
            } hints = {2, 0, 0, 0, 0};
            
            XChangeProperty(win->display, win->window, mwm_hints, mwm_hints, 
                           32, PropModeReplace, (unsigned char*)&hints, 5);
        }
    }
    
    // Set window title
    if (config->title) {
        XStoreName(win->display, win->window, config->title);
    }

    // IMPORTANT: Tell the WM NOT to give focus to this window.
    // This is critical for a virtual keyboard.
    XWMHints *wm_hints = XAllocWMHints();
    if (wm_hints) {
        wm_hints->flags = InputHint;
        wm_hints->input = False;
        XSetWMHints(win->display, win->window, wm_hints);
        XFree(wm_hints);
    }
    
    // Create graphics context
    win->gc = XCreateGC(win->display, win->window, 0, NULL);
    
    // Select input events
    long event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask | 
                     Button1MotionMask | StructureNotifyMask;
    XSelectInput(win->display, win->window, event_mask);
    
    // Set up WM_DELETE_WINDOW protocol
    win->wm_delete_window = get_atom(win, "WM_DELETE_WINDOW");
    XSetWMProtocols(win->display, win->window, &win->wm_delete_window, 1);
    
    // Set opacity if requested (requires composite manager)
    if (config->opacity < 1.0) {
        Atom net_wm_window_opacity = get_atom(win, "_NET_WM_WINDOW_OPACITY");
        if (net_wm_window_opacity != None) {
            unsigned long opacity = (unsigned long)(0xFFFFFFFFUL * config->opacity);
            XChangeProperty(win->display, win->window, net_wm_window_opacity,
                           XA_CARDINAL, 32, PropModeReplace,
                           (unsigned char*)&opacity, 1);
        }
    }
    
    win->event_callback = NULL;
    win->event_user_data = NULL;
    
    return win;
}

void x11_window_set_event_callback(X11Window *window, WindowEventCallback callback, void *user_data) {
    if (!window) return;
    
    window->event_callback = callback;
    window->event_user_data = user_data;
}

Display* x11_window_get_display(X11Window *window) {
    return window ? window->display : NULL;
}

Window x11_window_get_id(X11Window *window) {
    return window ? window->window : 0;
}

Visual* x11_window_get_visual(X11Window *window) {
    return window ? window->visual : NULL;
}

int x11_window_get_depth(X11Window *window) {
    return window ? window->depth : 0;
}

Pixmap x11_window_create_pixmap(X11Window *window, int width, int height) {
    if (!window || !window->display || width <= 0 || height <= 0) {
        return 0;
    }
    
    return XCreatePixmap(window->display, window->window, width, height, window->depth);
}

void x11_window_copy_area(X11Window *window, Pixmap src, 
                          int src_x, int src_y, int src_width, int src_height,
                          int dest_x, int dest_y) {
    if (!window || !window->display || src == 0) return;
    
    XCopyArea(window->display, src, window->window, window->gc,
              src_x, src_y, src_width, src_height, dest_x, dest_y);
}

void x11_window_move(X11Window *window, int x, int y) {
    if (!window || !window->display) return;

    XMoveWindow(window->display, window->window, x, y);
}

void x11_window_get_position(X11Window *window, int *x, int *y) {
    if (!window || !window->display) {
        if (x) *x = 0;
        if (y) *y = 0;
        return;
    }

    Window child;
    int root_x, root_y;
    XTranslateCoordinates(window->display, window->window, DefaultRootWindow(window->display),
                          0, 0, &root_x, &root_y, &child);
    
    if (x) *x = root_x;
    if (y) *y = root_y;
}

void x11_window_resize(X11Window *window, int width, int height) {
    if (!window || !window->display || width <= 0 || height <= 0) return;
    XResizeWindow(window->display, window->window, width, height);
    window->width = width;
    window->height = height;
}

void x11_window_move_resize(X11Window *window, int x, int y, int width, int height) {
    if (!window || !window->display || width <= 0 || height <= 0) return;
    XMoveResizeWindow(window->display, window->window, x, y, width, height);
    window->width = width;
    window->height = height;
}

void x11_window_get_size(X11Window *window, int *width, int *height) {
    if (!window) return;
    if (width) *width = window->width;
    if (height) *height = window->height;
}

void x11_window_set_title(X11Window *window, const char *title) {
    if (!window || !window->display || !title) return;
    
    XStoreName(window->display, window->window, title);
}

void x11_window_set_opacity(X11Window *win, double opacity) {
    if (!win) return;
    
    Atom opacity_atom = get_atom(win, "_NET_WM_WINDOW_OPACITY");
    if (opacity_atom != None) {
        unsigned long value = (unsigned long)(opacity * 0xFFFFFFFF);
        XChangeProperty(win->display, win->window, opacity_atom, XA_CARDINAL, 32, 
                        PropModeReplace, (unsigned char*)&value, 1);
    }
}

void x11_window_set_always_on_top(X11Window *win, bool on_top) {
    if (!win) return;
    
    Atom wm_state = get_atom(win, "_NET_WM_STATE");
    Atom wm_above = get_atom(win, "_NET_WM_STATE_ABOVE");
    
    if (wm_state != None && wm_above != None) {
        XEvent xev;
        memset(&xev, 0, sizeof(xev));
        xev.type = ClientMessage;
        xev.xclient.window = win->window;
        xev.xclient.message_type = wm_state;
        xev.xclient.format = 32;
        xev.xclient.data.l[0] = on_top ? 1 : 0; // 1 = _NET_WM_STATE_ADD
        xev.xclient.data.l[1] = wm_above;
        xev.xclient.data.l[2] = 0;
        
        XSendEvent(win->display, DefaultRootWindow(win->display), False,
                   SubstructureRedirectMask | SubstructureNotifyMask, &xev);
    }
}

void x11_window_show(X11Window *window) {
    if (!window || !window->display) return;
    
    XMapWindow(window->display, window->window);
    XFlush(window->display);
}

void x11_window_hide(X11Window *window) {
    if (!window || !window->display) return;
    
    XUnmapWindow(window->display, window->window);
    XFlush(window->display);
}

static void process_xevent(X11Window *window, XEvent *xev) {
    if (!window || !xev || !window->event_callback) return;
    
    WindowEvent event = {0};
    
    switch (xev->type) {
        case Expose:
            event.type = WINDOW_EVENT_EXPOSE;
            event.x = xev->xexpose.x;
            event.y = xev->xexpose.y;
            event.width = xev->xexpose.width;
            event.height = xev->xexpose.height;
            break;
            
        case ButtonPress:
            event.type = WINDOW_EVENT_BUTTON_PRESS;
            event.x = xev->xbutton.x;
            event.y = xev->xbutton.y;
            event.root_x = xev->xbutton.x_root;
            event.root_y = xev->xbutton.y_root;
            event.button = (MouseButton)xev->xbutton.button;
            event.state = xev->xbutton.state;
            break;
            
        case ButtonRelease:
            event.type = WINDOW_EVENT_BUTTON_RELEASE;
            event.x = xev->xbutton.x;
            event.y = xev->xbutton.y;
            event.root_x = xev->xbutton.x_root;
            event.root_y = xev->xbutton.y_root;
            event.button = (MouseButton)xev->xbutton.button;
            event.state = xev->xbutton.state;
            break;
            
        case MotionNotify:
            event.type = WINDOW_EVENT_MOTION;
            event.x = xev->xmotion.x;
            event.y = xev->xmotion.y;
            event.root_x = xev->xmotion.x_root;
            event.root_y = xev->xmotion.y_root;
            event.state = xev->xmotion.state;
            break;
            
        case ClientMessage:
            if ((Atom)xev->xclient.data.l[0] == window->wm_delete_window) {
                event.type = WINDOW_EVENT_CLOSE;
            }
            break;
            
        case ConfigureNotify:
            event.type = WINDOW_EVENT_RESIZE;
            event.width = xev->xconfigure.width;
            event.height = xev->xconfigure.height;
            break;
            
        default:
            return; // Unknown event type
    }
    
    window->event_callback(window, &event, window->event_user_data);
}

bool x11_window_process_events(X11Window *window) {
    if (!window || !window->display) return false;
    
    XEvent xev;
    bool had_events = false;
    
    while (XPending(window->display)) {
        XNextEvent(window->display, &xev);
        process_xevent(window, &xev);
        had_events = true;
    }
    
    return had_events;
}

#include <errno.h>

bool x11_window_wait_event(X11Window *window, int timeout_ms) {
    if (!window || !window->display) return false;
    
    if (timeout_ms > 0) {
        fd_set fds;
        FD_ZERO(&fds);
        int x11_fd = ConnectionNumber(window->display);
        FD_SET(x11_fd, &fds);
        
        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        
        int ret = select(x11_fd + 1, &fds, NULL, NULL, &tv);
        if (ret > 0) {
            return x11_window_process_events(window);
        } else if (ret < 0 && errno != EINTR) {
            // Real error, not just a signal
            return false;
        }
        return false;
    } else {
        // Blocking wait
        XEvent xev;
        XNextEvent(window->display, &xev);
        process_xevent(window, &xev);
        return true;
    }
}

void x11_window_close(X11Window *window) {
    if (!window || !window->display) return;
    
    XDestroyWindow(window->display, window->window);
    window->window = 0;
}

void x11_window_destroy(X11Window *window) {
    if (!window) return;
    
    if (window->gc) {
        XFreeGC(window->display, window->gc);
    }
    
    if (window->window) {
        XDestroyWindow(window->display, window->window);
    }
    
    if (window->display && window->owns_display) {
        XCloseDisplay(window->display);
    }
    
    free(window);
}