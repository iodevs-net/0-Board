// SPDX-License-Identifier: MIT — see LICENSE file

#ifndef LAYOUT_H
#define LAYOUT_H

#include <X11/Xlib.h>
#include <stdbool.h>
#include "constants.h"

#define KEYFLAG_NORMAL   0
#define KEYFLAG_SHIFT    1
#define KEYFLAG_SYMBOLS  2
#define KEYFLAG_MODIFIER 4
#define KEYFLAG_CTRL     8
#define KEYFLAG_ALT      16
#define KEYFLAG_META     32
#define KEYFLAG_FN       64

typedef struct {
    KeySym normal;
    KeySym shifted;
    KeySym altgr;
    KeySym fn_keysym;       // keysym when FN layer active (0 = same as normal)
    const char *fn_label;   // label when FN layer active (NULL = same as label)
    const char *label;
    const char *shifted_label; // NULL if same or auto-uppercase
    bool new_row;
    int width_weight;
    int flags;
} KeyDef;

typedef struct {
    const char *name;
    KeyDef *keys;
    int num_keys;
} Layout;

Layout* layout_get_default();
void layout_init(Layout *l);

#endif