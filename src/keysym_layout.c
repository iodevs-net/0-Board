// SPDX-License-Identifier: MIT — see LICENSE file

#include "keysym_layout.h"
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <stdlib.h>

KeyCode keysym_layout_resolve(Display *dpy, KeySym target, int *modifiers) {
    if (!dpy || !modifiers) return 0;
    *modifiers = 0;

    int min_kc, max_kc;
    XDisplayKeycodes(dpy, &min_kc, &max_kc);
    if (min_kc <= 0 || max_kc <= min_kc) return 0;

    // XKB: iterate keycodes, check all levels using Xkb function
    // Works correctly for all layouts (US, ES, DE, etc.)
    for (int kc = min_kc; kc <= max_kc; kc++) {
        if (XkbKeycodeToKeysym(dpy, kc, 0, 0) == target) return kc;        // none
        if (XkbKeycodeToKeysym(dpy, kc, 0, 1) == target) { *modifiers = 1; return kc; }  // Shift
        if (XkbKeycodeToKeysym(dpy, kc, 0, 2) == target) { *modifiers = 4; return kc; }  // AltGr
        if (XkbKeycodeToKeysym(dpy, kc, 0, 3) == target) { *modifiers = 5; return kc; }  // Shift+AltGr
    }

    // Fallback: core X11 keymap
    int per_kc;
    KeySym *map = XGetKeyboardMapping(dpy, min_kc, max_kc - min_kc + 1, &per_kc);
    if (map) {
        for (int kc = min_kc; kc <= max_kc; kc++) {
            int idx = (kc - min_kc) * per_kc;
            for (int level = 0; level < per_kc; level++) {
                if (map[idx + level] == target) {
                    if (level & 1) *modifiers |= 1;
                    if (level & 2) *modifiers |= 4;
                    XFree(map);
                    return kc;
                }
            }
        }
        XFree(map);
    }

    return 0;
}
