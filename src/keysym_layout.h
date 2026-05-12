// SPDX-License-Identifier: MIT — see LICENSE file

#ifndef KEYSYM_LAYOUT_H
#define KEYSYM_LAYOUT_H

#include <X11/Xlib.h>

// Queries the current X11 keyboard layout and finds the correct keycode
// + modifier mask for a given keysym. This works for any layout (US, ES, etc.)
// Returns keycode, sets *modifiers to the mask (1=Shift, 4=AltGr)
// Returns 0 if keysym cannot be mapped on current layout
KeyCode keysym_layout_resolve(Display *dpy, KeySym target, int *modifiers);

#endif
