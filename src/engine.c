// SPDX-License-Identifier: MIT — see LICENSE file

#include "engine.h"
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XTest.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "keysym_util.h"
#include "keysym_layout.h"

struct Engine {
    Display *display;
    bool use_xtest;
    int event_delay_us;
    bool owns_display;
};

Engine* engine_create(EngineConfig *config) {
    if (!config) return NULL;

    Engine *engine = malloc(sizeof(Engine));
    if (!engine) return NULL;

    engine->display = XOpenDisplay(NULL);
    if (!engine->display) {
        fprintf(stderr, "engine_create: failed to open independent X11 display\n");
        free(engine);
        return NULL;
    }

    engine->use_xtest = config->use_xtest;
    engine->event_delay_us = config->event_delay_us;
    engine->owns_display = true;

    return engine;
}

int engine_send_key(Engine *engine, KeySym keysym, bool pressed) {
    return engine_send_key_ex(engine, keysym, pressed, 0);
}

static KeyCode keysym_to_keycode(Display *dpy, KeySym sym, int *modifiers) {
    int layout_mods = 0;
    KeyCode kc = keysym_layout_resolve(dpy, sym, &layout_mods);
    if (kc) {
        if (sym < 0xff00) {
            *modifiers &= ~1; // Clear Shift from virtual layer selection
        }
        *modifiers |= layout_mods;
        return kc;
    }
    // Fallback for layouts where lookup fails
    kc = XKeysymToKeycode(dpy, sym);
    if (kc) return kc;
    if (sym >= XK_A && sym <= XK_Z) {
        kc = XKeysymToKeycode(dpy, sym - XK_A + XK_a);
        if (kc) {
            if (sym < 0xff00) *modifiers &= ~1;
            *modifiers |= 1;
            return kc;
        }
    }
    KeySym base = keysym_get_base(sym);
    if (base) {
        kc = XKeysymToKeycode(dpy, base);
        if (kc) {
            if (sym < 0xff00) *modifiers &= ~1;
            *modifiers |= 1;
            return kc;
        }
    }
    return 0;
}


static int inject_key_sequence(Display *dpy, KeyCode kc, int ob_mods, Bool pressed) {
    // ob_mods: 0=none, 1=Shift, 4=AltGr, 5=Shift+AltGr, etc.
    // Uses XTest for Shift (always works) and XkbLockModifiers for
    // non-Shift modifiers (Mod4/Mod5) which XTest can't always activate.

    if (pressed) {
        // For Shift, use XTest (works reliably)
        if (ob_mods & 1) {
            KeyCode skc = XKeysymToKeycode(dpy, XK_Shift_L);
            if (skc) XTestFakeKeyEvent(dpy, skc, True, 0);
        }
        // For AltGr (Mod5), use XkbLockModifiers or try Mode_switch
        if (ob_mods & 4) {
            // Try locking Mod5 (AltGr) directly - bypasses keycode issues
            XkbLockModifiers(dpy, XkbUseCoreKbd, Mod5Mask, Mod5Mask);
            XFlush(dpy);
            usleep(10000);
        }
        XTestFakeKeyEvent(dpy, kc, True, 0);
        XFlush(dpy);
    } else {
        XTestFakeKeyEvent(dpy, kc, False, 0);
        // Release AltGr
        if (ob_mods & 4) {
            XkbLockModifiers(dpy, XkbUseCoreKbd, Mod5Mask, 0);
            XFlush(dpy);
        }
        // Release Shift
        if (ob_mods & 1) {
            KeyCode skc = XKeysymToKeycode(dpy, XK_Shift_L);
            if (skc) XTestFakeKeyEvent(dpy, skc, False, 0);
        }
        XFlush(dpy);  // ENVIAR release al servidor X
    }
    return 0;
}

int engine_send_key_ex(Engine *engine, KeySym keysym, bool pressed, int modifiers) {
    if (!engine || !engine->display || !engine->use_xtest || keysym == 0)
        return -1;

    KeyCode kc = keysym_to_keycode(engine->display, keysym, &modifiers);
    if (!kc) return -1;

    return inject_key_sequence(engine->display, kc, modifiers, pressed);
}

void engine_flush(Engine *engine) {
    if (engine && engine->display) {
        XFlush(engine->display);
    }
}

int engine_send_mouse_click(Engine *engine, int button) {
    if (!engine || !engine->display || !engine->use_xtest)
        return -1;
    if (button < 1 || button > 3)
        return -1;
    XTestFakeButtonEvent(engine->display, button, True, 0);
    XFlush(engine->display);
    usleep(engine->event_delay_us);
    XTestFakeButtonEvent(engine->display, button, False, 0);
    XFlush(engine->display);
    return 0;
}

int engine_send_scroll(Engine *engine, int direction) {
    if (!engine || !engine->display || !engine->use_xtest)
        return -1;
    if (direction < 4 || direction > 5)
        return -1;
    XTestFakeButtonEvent(engine->display, direction, True, 0);
    XTestFakeButtonEvent(engine->display, direction, False, 0);
    XFlush(engine->display);
    return 0;
}

void engine_destroy(Engine *engine) {
    if (!engine) return;

    if (engine->display && engine->owns_display) {
        XCloseDisplay(engine->display);
    }

    free(engine);
}
