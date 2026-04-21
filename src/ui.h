// ui.h - Interfaz del motor de renderizado optimizado (Zero-Waste Cairo)
#ifndef XVKBD_UI_H
#define XVKBD_UI_H

#include <X11/Xlib.h>
#include <stdbool.h>
#include "layout.h"

typedef struct {
    double win_radius;
    double key_radius;
    double win_opacity;
    double key_opacity;
    const char *font_family;
    int font_size;
    bool show_titlebar;
} Theme;

// Flags pre-calculados para evitar strcmp en el hot-path
#define KEYFLAG_NORMAL   0
#define KEYFLAG_SHIFT    1
#define KEYFLAG_SYMBOLS  2
#define KEYFLAG_MODIFIER 4  // No envía KeySym, solo cambia estado

typedef struct {
    int x, y, w, h;
    KeyDef *key;
    int flags;  // KEYFLAG_* pre-calculados
} KeyRect;

typedef enum {
    LAYER_NORMAL,
    LAYER_SHIFT,
    LAYER_SYMBOLS
} KeyboardLayer;

// Layout pre-computado (calculado una sola vez en init)
typedef struct {
    int start;   // Índice de inicio en el array de teclas
    int count;   // Número de teclas en la fila
    int weight;  // Suma de pesos de la fila
} RowInfo;

typedef struct {
    Display *display;
    Window window;
    Visual *visual;
    int width, height;
    Theme theme;
    Layout *layout;

    // Rectángulos pre-calculados para hit-testing
    KeyRect rects[128];
    int num_rects;

    // Grilla pre-computada (NO se recalcula en cada render)
    RowInfo rows[10];
    int num_rows;

    // Estado de la capa activa
    KeyboardLayer current_layer;

    // Cache: Pixmap off-screen para evitar re-render constante
    Pixmap backbuffer;
    bool dirty;  // Solo re-renderizar cuando cambia algo
} UIState;

void ui_init(UIState *state, Layout *layout);
void ui_loop(UIState *state);

#endif
