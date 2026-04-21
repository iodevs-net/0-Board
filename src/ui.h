#ifndef XVKBD_UI_H
#define XVKBD_UI_H

#include <X11/Xlib.h>
#include <stdbool.h>
#include "layout.h"

typedef struct {
    unsigned long bg_color;
    float win_opacity;
    float key_opacity;
    int win_radius;
    int key_radius;
    bool show_titlebar;
    const char *font_family;
    int font_size;
} Theme;

typedef struct {
    int x, y, w, h;
    KeyDef *key;
} KeyRect;

typedef struct {
    Display *display;
    Window window;
    Visual *visual;
    Theme theme;
    Layout *layout;
    int width, height;
    KeyRect rects[64];
    int num_rects;
} UIState;

void ui_init(UIState *state, Layout *layout);
void ui_loop(UIState *state);

#endif
