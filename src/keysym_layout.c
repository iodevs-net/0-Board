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

    // Primary: iterate keycodes via XKB (handles all levels correctly)
    for (int kc = min_kc; kc <= max_kc; kc++) {
        // Check level 0 (plain key press, no modifier)
        if (XkbKeycodeToKeysym(dpy, kc, 0, 0) == target) {
            *modifiers = 0;
            return kc;
        }
    }

    // Level 0 not found. Try other levels (Shift, AltGr, Shift+AltGr).
    // Use XkbKeysymToModifiers to get the EXACT modifier mask.
    unsigned int x11_mods = XkbKeysymToModifiers(dpy, target);
    int needs_shift = (x11_mods & ShiftMask) != 0;
    int needs_altgr = (x11_mods & Mod5Mask) != 0;
    int needs_alt   = (x11_mods & Mod1Mask) != 0;

    for (int kc = min_kc; kc <= max_kc; kc++) {
        // Check level 1 (Shift)
        if (needs_shift && !needs_altgr && !needs_alt) {
            if (XkbKeycodeToKeysym(dpy, kc, 0, 1) == target) {
                *modifiers = 1; // Shift
                return kc;
            }
        }
        // Check level 2 (AltGr)
        if (needs_altgr && !needs_shift) {
            if (XkbKeycodeToKeysym(dpy, kc, 0, 2) == target) {
                *modifiers = 4; // AltGr
                return kc;
            }
        }
        // Check level 3 (Shift+AltGr)
        if (needs_shift && needs_altgr) {
            if (XkbKeycodeToKeysym(dpy, kc, 0, 3) == target) {
                *modifiers = 5; // Shift+AltGr
                return kc;
            }
        }
    }

    // Phase 3: Fallback — try all levels without modifier check
    for (int kc = min_kc; kc <= max_kc; kc++) {
        for (int level = 0; level < 4; level++) {
            KeySym ks = XkbKeycodeToKeysym(dpy, kc, 0, level);
            if (ks == target) {
                if (level & 1) *modifiers |= 1;
                if (level & 2) *modifiers |= 4;
                return kc;
            }
        }
    }

    // Phase 4: Core X11 fallback
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
