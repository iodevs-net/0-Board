// ui.c - Motor de renderizado Cairo ultra-optimizado
// Patrón: Dirty-Flag + Pixmap Backbuffer + Pre-compute
// Objetivo: 0% CPU en reposo, render solo cuando cambia la capa.

#include "ui.h"
#include "engine.h"
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// ── Utilidades de dibujo ──

static void draw_rounded_rect(cairo_t *cr, double x, double y,
                               double w, double h, double r) {
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - r, y + r,     r, -1.5708, 0);
    cairo_arc(cr, x + w - r, y + h - r, r, 0,       1.5708);
    cairo_arc(cr, x + r,     y + h - r, r, 1.5708,  3.14159);
    cairo_arc(cr, x + r,     y + r,     r, 3.14159, 4.71239);
    cairo_close_path(cr);
}

// ── Tabla de traducción de KeySym a etiqueta legible ──
// Se usa un array estático para evitar strcmp en cadena.

typedef struct { KeySym ks; const char *label; } SymLabel;

static const SymLabel sym_table[] = {
    {XK_comma,     ","}, {XK_period,      "."}, {XK_slash,       "/"},
    {XK_backslash, "\\"}, {XK_bracketleft, "["}, {XK_bracketright, "]"},
    {XK_braceleft, "{"}, {XK_braceright,  "}"}, {XK_less,        "<"},
    {XK_greater,   ">"}, {XK_quotedbl,    "\""},{XK_apostrophe,  "'"},
    {XK_underscore,"_"}, {XK_equal,       "="}, {XK_plus,        "+"},
    {XK_minus,     "-"}, {XK_asterisk,    "*"}, {XK_ampersand,   "&"},
    {XK_percent,   "%"}, {XK_dollar,      "$"}, {XK_numbersign,  "#"},
    {XK_at,        "@"}, {XK_exclam,      "!"}, {XK_question,    "?"},
    {XK_bar,       "|"}, {XK_asciitilde,  "~"}, {XK_grave,       "`"},
    {XK_colon,     ":"}, {XK_semicolon,   ";"}, {XK_parenleft,   "("},
    {XK_parenright,")"}, {XK_dead_tilde,  "~"},
    {0xf1,         "ñ"}, {0xd1,           "Ñ"},
    {0, NULL}  // Centinela
};

// Buscar etiqueta por KeySym (O(n) pero n=~30, más rápido que strcmp en cadena)
static const char* keysym_to_label(KeySym ks) {
    for (const SymLabel *s = sym_table; s->label; s++) {
        if (s->ks == ks) return s->label;
    }
    return NULL;
}

// Obtener la etiqueta visual según la capa activa
static const char* get_key_label(KeyDef *k, KeyboardLayer layer, int flags) {
    // Teclas modificadoras siempre muestran su label fijo
    if (flags & KEYFLAG_MODIFIER) return k->label;

    KeySym ks = k->normal;
    if (layer == LAYER_SHIFT)   ks = k->shifted;
    if (layer == LAYER_SYMBOLS) ks = k->altgr;
    if (ks == 0) ks = k->normal;

    // Buscar en tabla rápida
    const char *found = keysym_to_label(ks);
    if (found) return found;

    // Intentar XKeysymToString (para letras simples como 'a', 'A', '1'...)
    const char *name = XKeysymToString(ks);
    if (name && strlen(name) == 1) return name;

    return k->label;
}

// ── Inicialización: pre-computa toda la geometría ──

static void precompute_grid(UIState *state) {
    state->num_rows = 0;
    for (int i = 0; i < state->layout->num_keys; i++) {
        if (state->layout->keys[i].new_row || i == 0) {
            RowInfo *ri = &state->rows[state->num_rows];
            ri->start  = i;
            ri->count  = 0;
            ri->weight = 0;
            state->num_rows++;
        }
        RowInfo *ri = &state->rows[state->num_rows - 1];
        ri->count++;
        ri->weight += state->layout->keys[i].width_weight;
    }

    // Pre-calcular rectángulos y flags
    int pad = 20, gap = 7, row_h = 56;
    int total_w = state->width - 2 * pad;
    state->num_rects = 0;
    int cur_y = pad;

    for (int r = 0; r < state->num_rows; r++) {
        RowInfo *ri = &state->rows[r];
        int cur_x = pad;
        int usable_w = total_w - (gap * (ri->count - 1));

        for (int j = 0; j < ri->count; j++) {
            int idx = ri->start + j;
            KeyDef *k = &state->layout->keys[idx];
            int kw = (k->width_weight * usable_w) / ri->weight;

            KeyRect *kr = &state->rects[state->num_rects];
            kr->x = cur_x;
            kr->y = cur_y;
            kr->w = kw;
            kr->h = row_h;
            kr->key = k;

            // Pre-calcular flags (elimina strcmp del hot-path)
            kr->flags = KEYFLAG_NORMAL;
            if (strcmp(k->label, "shift") == 0)  kr->flags = KEYFLAG_SHIFT | KEYFLAG_MODIFIER;
            else if (strcmp(k->label, "?123") == 0)  kr->flags = KEYFLAG_SYMBOLS | KEYFLAG_MODIFIER;
            else if (strcmp(k->label, "caps") == 0 || strcmp(k->label, "del") == 0 ||
                     strcmp(k->label, "return") == 0 || strcmp(k->label, "tab") == 0 ||
                     strcmp(k->label, "ctrl") == 0 || strcmp(k->label, "opt") == 0 ||
                     strcmp(k->label, "cmd") == 0 || strcmp(k->label, " ") == 0)
                kr->flags = KEYFLAG_MODIFIER;

            cur_x += kw + gap;
            state->num_rects++;
        }
        cur_y += row_h + gap;
    }
}

void ui_init(UIState *state, Layout *layout) {
    state->display = XOpenDisplay(NULL);
    state->layout = layout;
    state->width = 1050;
    state->height = 370;
    state->current_layer = LAYER_NORMAL;
    state->dirty = true;

    state->theme.win_radius = 18;
    state->theme.key_radius = 6;
    state->theme.win_opacity = 0.94;
    state->theme.key_opacity = 1.0;
    state->theme.font_family = "Inter, Sans";
    state->theme.font_size = 14;
    state->theme.show_titlebar = false;

    Window root = DefaultRootWindow(state->display);
    XVisualInfo vinfo;
    XMatchVisualInfo(state->display, DefaultScreen(state->display), 32, TrueColor, &vinfo);
    state->visual = vinfo.visual;

    XSetWindowAttributes attr;
    attr.colormap = XCreateColormap(state->display, root, state->visual, AllocNone);
    attr.border_pixel = 0;
    attr.background_pixel = 0;
    attr.override_redirect = !state->theme.show_titlebar;
    // Activar backing store del servidor X (reduce Expose innecesarios)
    attr.backing_store = WhenMapped;

    state->window = XCreateWindow(state->display, root, 150, 150,
                                  state->width, state->height, 0,
                                  vinfo.depth, InputOutput, state->visual,
                                  CWColormap | CWBorderPixel | CWBackPixel |
                                  CWOverrideRedirect | CWBackingStore, &attr);

    // Crear Pixmap backbuffer (mismo depth que la ventana ARGB)
    state->backbuffer = XCreatePixmap(state->display, state->window,
                                      state->width, state->height, vinfo.depth);

    XSelectInput(state->display, state->window,
                 ExposureMask | ButtonPressMask | ButtonReleaseMask | Button1MotionMask);
    XMapWindow(state->display, state->window);

    // Pre-computar toda la geometría (se hace UNA SOLA VEZ)
    precompute_grid(state);
}

// ── Render al backbuffer (solo cuando dirty=true) ──

static void render_to_backbuffer(UIState *state) {
    cairo_surface_t *surf = cairo_xlib_surface_create(
        state->display, state->backbuffer, state->visual,
        state->width, state->height);
    cairo_t *cr = cairo_create(surf);

    // Fondo del teclado
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(cr, 0.1, 0.1, 0.1, state->theme.win_opacity);
    draw_rounded_rect(cr, 0, 0, state->width, state->height, state->theme.win_radius);
    cairo_fill(cr);

    // Configurar fuente UNA VEZ por render (no por cada tecla)
    cairo_select_font_face(cr, state->theme.font_family,
                           CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, state->theme.font_size);

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    // Dibujar teclas usando rectángulos pre-calculados
    for (int i = 0; i < state->num_rects; i++) {
        KeyRect *kr = &state->rects[i];

        // Color: resaltar teclas de capa activa
        bool active = false;
        if ((kr->flags & KEYFLAG_SHIFT) && state->current_layer == LAYER_SHIFT) active = true;
        if ((kr->flags & KEYFLAG_SYMBOLS) && state->current_layer == LAYER_SYMBOLS) active = true;

        if (active) cairo_set_source_rgba(cr, 0.38, 0.38, 0.38, 1.0);
        else        cairo_set_source_rgba(cr, 0.22, 0.22, 0.22, 1.0);

        draw_rounded_rect(cr, kr->x, kr->y, kr->w, kr->h, state->theme.key_radius);
        cairo_fill(cr);

        // Etiqueta centrada
        const char *label = get_key_label(kr->key, state->current_layer, kr->flags);
        cairo_set_source_rgb(cr, 0.95, 0.95, 0.95);
        cairo_text_extents_t ext;
        cairo_text_extents(cr, label, &ext);
        cairo_move_to(cr,
                      kr->x + kr->w / 2.0 - ext.width / 2.0 - ext.x_bearing,
                      kr->y + kr->h / 2.0 - ext.height / 2.0 - ext.y_bearing);
        cairo_show_text(cr, label);
    }

    cairo_destroy(cr);
    cairo_surface_destroy(surf);
    state->dirty = false;
}

// Copiar backbuffer a ventana (operación ultra-rápida, sin Cairo)
static void blit_to_window(UIState *state) {
    GC gc = XCreateGC(state->display, state->window, 0, NULL);
    XCopyArea(state->display, state->backbuffer, state->window, gc,
              0, 0, state->width, state->height, 0, 0);
    XFreeGC(state->display, gc);
}

// ── Loop de eventos principal ──

void ui_loop(UIState *state) {
    XEvent ev;
    int drag_x = 0, drag_y = 0;
    bool dragging = false;

    while (1) {
        XNextEvent(state->display, &ev);

        if (ev.type == Expose && ev.xexpose.count == 0) {
            // Solo re-renderizar si hay cambios; si no, copiar cache
            if (state->dirty) render_to_backbuffer(state);
            blit_to_window(state);
        }

        if (ev.type == ButtonPress && ev.xbutton.button == 1) {
            bool on_key = false;
            int mx = ev.xbutton.x, my = ev.xbutton.y;

            for (int i = 0; i < state->num_rects; i++) {
                KeyRect *kr = &state->rects[i];
                if (mx >= kr->x && mx <= kr->x + kr->w &&
                    my >= kr->y && my <= kr->y + kr->h) {
                    on_key = true;

                    if (kr->flags & KEYFLAG_SHIFT) {
                        state->current_layer = (state->current_layer == LAYER_SHIFT)
                            ? LAYER_NORMAL : LAYER_SHIFT;
                        state->dirty = true;
                    } else if (kr->flags & KEYFLAG_SYMBOLS) {
                        state->current_layer = (state->current_layer == LAYER_SYMBOLS)
                            ? LAYER_NORMAL : LAYER_SYMBOLS;
                        state->dirty = true;
                    } else {
                        KeySym ks = kr->key->normal;
                        if (state->current_layer == LAYER_SHIFT)   ks = kr->key->shifted;
                        if (state->current_layer == LAYER_SYMBOLS) ks = kr->key->altgr;

                        if (ks != XK_VoidSymbol && ks != 0) {
                            engine_send_key(ks, true);
                            engine_send_key(ks, false);
                        }
                        // Auto-revert shift (comportamiento móvil)
                        if (state->current_layer == LAYER_SHIFT) {
                            state->current_layer = LAYER_NORMAL;
                            state->dirty = true;
                        }
                    }

                    if (state->dirty) {
                        render_to_backbuffer(state);
                        blit_to_window(state);
                    }
                    break;
                }
            }
            if (!on_key) {
                dragging = true;
                drag_x = mx;
                drag_y = my;
            }
        }

        if (ev.type == ButtonRelease && ev.xbutton.button == 1)
            dragging = false;

        if (ev.type == MotionNotify && dragging)
            XMoveWindow(state->display, state->window,
                        ev.xmotion.x_root - drag_x, ev.xmotion.y_root - drag_y);
    }
}
