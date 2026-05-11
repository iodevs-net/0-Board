// SPDX-License-Identifier: MIT — see LICENSE file

#include "engine.h"
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "keysym_util.h"

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

int engine_send_key_ex(Engine *engine, KeySym keysym, bool pressed, int modifiers) {
    if (!engine || !engine->display || keysym == 0) {
        return -1;
    }

    if (!engine->use_xtest) {
        fprintf(stderr, "engine_send_key: XTest required\n");
        return -1;
    }

    // Try direct keysym → keycode first
    KeyCode kc = XKeysymToKeycode(engine->display, keysym);

    // If no direct mapping, try base keysym + add modifiers
    // Example: XK_exclam may not have a keycode, but XK_1 does
    if (!kc) {
        KeySym base = keysym;
        // Strip shift: if uppercase letter, convert to lowercase
        if (keysym >= XK_A && keysym <= XK_Z) {
            base = keysym - XK_A + XK_a;
            modifiers |= 1; // Need Shift for uppercase
        }
        // For other shifted symbols, try the unshifted version
        // XK_exclam → XK_1, XK_at → XK_2, etc.
        else if (keysym >= XK_exclam && keysym <= XK_asciitilde) {
            KeySym found = keysym_get_base(keysym);
            if (found) {
                base = found;
                modifiers |= 1;
            }
        }
        kc = XKeysymToKeycode(engine->display, base);
        if (!kc) return -1; // Truly unmappable
    }

    // Modifiers mask: 1=Shift, 2=Ctrl, 4=Alt, 8=Meta
    KeyCode shift_kc = XKeysymToKeycode(engine->display, XK_Shift_L);
    KeyCode ctrl_kc  = XKeysymToKeycode(engine->display, XK_Control_L);
    KeyCode alt_kc   = XKeysymToKeycode(engine->display, XK_Alt_L);
    KeyCode meta_kc  = XKeysymToKeycode(engine->display, XK_Super_L);

    bool need_shift = (modifiers & 1);
    bool need_ctrl  = (modifiers & 2);
    bool need_alt   = (modifiers & 4);
    bool need_meta  = (modifiers & 8);

    // If key IS a modifier, don't double-press it
    if (keysym == XK_Shift_L || keysym == XK_Shift_R) need_shift = false;
    if (keysym == XK_Control_L || keysym == XK_Control_R) need_ctrl = false;
    if (keysym == XK_Alt_L || keysym == XK_Alt_R) need_alt = false;
    if (keysym == XK_Super_L || keysym == XK_Super_R) need_meta = false;

    if (pressed) {
        if (need_shift) XTestFakeKeyEvent(engine->display, shift_kc, True, 0);
        if (need_ctrl)  XTestFakeKeyEvent(engine->display, ctrl_kc,  True, 0);
        if (need_alt)   XTestFakeKeyEvent(engine->display, alt_kc,   True, 0);
        if (need_meta)  XTestFakeKeyEvent(engine->display, meta_kc,  True, 0);
    }

    XTestFakeKeyEvent(engine->display, kc, pressed, 0);

    if (!pressed) {
        if (need_shift) XTestFakeKeyEvent(engine->display, shift_kc, False, 0);
        if (need_ctrl)  XTestFakeKeyEvent(engine->display, ctrl_kc,  False, 0);
        if (need_alt)   XTestFakeKeyEvent(engine->display, alt_kc,   False, 0);
        if (need_meta)  XTestFakeKeyEvent(engine->display, meta_kc,  False, 0);
    }

    return 0;
}

void engine_flush(Engine *engine) {
    if (engine && engine->display) {
        XFlush(engine->display);
    }
}

void engine_destroy(Engine *engine) {
    if (!engine) return;

    if (engine->display && engine->owns_display) {
        XCloseDisplay(engine->display);
    }

    free(engine);
}
