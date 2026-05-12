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

    // Phase 1: Find the FIRST keycode for this keysym at ANY level.
    // Return keycode + level-based modifier.
    // Prefer simpler modifiers (level 0 best, then 1, 2, 3).
    for (int kc = min_kc; kc <= max_kc; kc++) {
        for (int level = 0; level < 4; level++) {
            if (XkbKeycodeToKeysym(dpy, kc, 0, level) == target) {
                int mods = 0;
                if (level & 1) mods |= 1;  // Shift
                if (level & 2) mods |= 4;  // AltGr
                *modifiers = mods;
                return kc;
            }
        }
    }

    // Phase 2: Core X11 fallback
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
