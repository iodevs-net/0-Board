// SPDX-License-Identifier: MIT — see LICENSE file

#include "engine.h"
#include <X11/Xlib.h>
#include <X11/keysym.h>
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
        *modifiers |= layout_mods;
        return kc;
    }
    // Fallback for layouts where lookup fails
    kc = XKeysymToKeycode(dpy, sym);
    if (kc) return kc;
    if (sym >= XK_A && sym <= XK_Z) {
        kc = XKeysymToKeycode(dpy, sym - XK_A + XK_a);
        if (kc) { *modifiers |= 1; return kc; }
    }
    KeySym base = keysym_get_base(sym);
    if (base) {
        kc = XKeysymToKeycode(dpy, base);
        if (kc) { *modifiers |= 1; return kc; }
    }
    return 0;
}

static int modifier_keys_for_mask(Display *dpy, int modifiers, KeyCode *out, int max, KeySym self) {
    int n = 0;
    if ((modifiers & 1) && self != XK_Shift_L && self != XK_Shift_R)
        if (n < max) out[n++] = XKeysymToKeycode(dpy, XK_Shift_L);
    if ((modifiers & 2) && self != XK_Control_L && self != XK_Control_R)
        if (n < max) out[n++] = XKeysymToKeycode(dpy, XK_Control_L);
    if ((modifiers & 4) && self != XK_Alt_L && self != XK_Alt_R) {
        KeyCode altgr = XKeysymToKeycode(dpy, XK_ISO_Level3_Shift);
        if (!altgr) altgr = XKeysymToKeycode(dpy, XK_Mode_switch);
        if (!altgr) altgr = XKeysymToKeycode(dpy, XK_Alt_R);
        if (!altgr) altgr = XKeysymToKeycode(dpy, XK_Alt_L);
        if (n < max) out[n++] = altgr;
    }
    if ((modifiers & 8) && self != XK_Super_L && self != XK_Super_R)
        if (n < max) out[n++] = XKeysymToKeycode(dpy, XK_Super_L);
    return n;
}

static int inject_key_sequence(Display *dpy, KeyCode kc, KeyCode *mods, int n, Bool pressed) {
    if (pressed) {
        for (int i = 0; i < n; i++)
            XTestFakeKeyEvent(dpy, mods[i], True, 0);
    }
    XTestFakeKeyEvent(dpy, kc, pressed, 0);
    if (!pressed) {
        for (int i = n - 1; i >= 0; i--)
            XTestFakeKeyEvent(dpy, mods[i], False, 0);
    }
    return 0;
}

int engine_send_key_ex(Engine *engine, KeySym keysym, bool pressed, int modifiers) {
    if (!engine || !engine->display || !engine->use_xtest || keysym == 0)
        return -1;

    KeyCode kc = keysym_to_keycode(engine->display, keysym, &modifiers);
    if (!kc) return -1;

    KeyCode mod_keys[4];
    int mod_count = modifier_keys_for_mask(engine->display, modifiers, mod_keys, 4, keysym);

    return inject_key_sequence(engine->display, kc, mod_keys, mod_count, pressed);
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

void engine_destroy(Engine *engine) {
    if (!engine) return;

    if (engine->display && engine->owns_display) {
        XCloseDisplay(engine->display);
    }

    free(engine);
}
