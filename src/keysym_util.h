// SPDX-License-Identifier: MIT — see LICENSE file

#ifndef KEYSYM_UTIL_H
#define KEYSYM_UTIL_H

#include <X11/Xlib.h>
#include <X11/keysym.h>

typedef struct {
    KeySym shifted;
    KeySym base;
} ShiftPair;

// Map: shifted → base + Shift modifier
// Used by both layout keyboard_state for layout matching and engine for key injection
#define SHIFT_PAIRS_COUNT 21
extern const ShiftPair g_shift_pairs[SHIFT_PAIRS_COUNT];

// Check if a keysym is a letter (A-Z/a-z/ñ)
int keysym_is_letter(KeySym sym);

// Check if a keysym is a standard shifted version of another key
// Returns the base keysym if found, 0 otherwise
KeySym keysym_get_base(KeySym sym);

#endif
