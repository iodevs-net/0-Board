/*
 * 0-Board Virtual Keyboard
 * Copyright (c) 2026 Leonardo Vergara <leonardovergaramarin@gmail.com>
 * Licensed under the MIT License.
 */
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

typedef struct {
    KeySym normal;
    KeySym shifted;
    KeySym altgr;
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