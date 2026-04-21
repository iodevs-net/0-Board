#ifndef XVKBD_LAYOUT_H
#define XVKBD_LAYOUT_H

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdbool.h>

typedef struct {
    const char *label;
    KeySym normal;
    KeySym shifted;
    KeySym altgr;
    int width_weight;
    bool new_row;
} KeyDef;

typedef struct {
    const char *name;
    KeyDef *keys;
    int num_keys;
} Layout;

extern Layout layout_es;
extern Layout layout_en;

#endif
