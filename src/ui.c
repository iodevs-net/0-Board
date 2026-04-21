// ui.c - Motor de renderizado Cairo (Apple Magic Keyboard Style)
// Cada fila se escala independientemente para ocupar el 100% del ancho.

#include "ui.h"
#include "engine.h"
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <stdio.h>

// Dibujar rectángulo con esquinas redondeadas
static void draw_rounded_rect(cairo_t *cr, double x, double y, double w, double h, double r) {
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - r, y + r,     r, -1.5708, 0);       // Arriba-derecha
    cairo_arc(cr, x + w - r, y + h - r, r, 0,       1.5708);  // Abajo-derecha
    cairo_arc(cr, x + r,     y + h - r, r, 1.5708,  3.14159); // Abajo-izquierda
    cairo_arc(cr, x + r,     y + r,     r, 3.14159, 4.71239); // Arriba-izquierda
    cairo_close_path(cr);
}

void ui_init(UIState *state, Layout *layout) {
    state->display = XOpenDisplay(NULL);
    state->layout = layout;
    state->width = 1050;
    state->height = 370;

    // Estilo Apple Magic Keyboard (Space Gray)
    state->theme.win_radius = 18;
    state->theme.key_radius = 5;
    state->theme.win_opacity = 0.92;
    state->theme.key_opacity = 1.0;
    state->theme.font_family = "Inter, Helvetica, Sans";
    state->theme.font_size = 13;
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

    state->window = XCreateWindow(state->display, root, 150, 150,
                                  state->width, state->height, 0,
                                  vinfo.depth, InputOutput, state->visual,
                                  CWColormap | CWBorderPixel | CWBackPixel | CWOverrideRedirect, &attr);

    XSelectInput(state->display, state->window,
                 ExposureMask | ButtonPressMask | ButtonReleaseMask | Button1MotionMask);
    XMapWindow(state->display, state->window);
}

static void render(UIState *state) {
    cairo_surface_t *surf = cairo_xlib_surface_create(
        state->display, state->window, state->visual, state->width, state->height);
    cairo_t *cr = cairo_create(surf);

    // Fondo del teclado (Space Gray)
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(cr, 0.11, 0.11, 0.11, state->theme.win_opacity);
    draw_rounded_rect(cr, 0, 0, state->width, state->height, state->theme.win_radius);
    cairo_fill(cr);

    int pad = 20;       // Margen exterior
    int gap = 6;        // Espacio entre teclas
    int row_h = 56;     // Altura de cada tecla
    int total_w = state->width - 2 * pad;  // Ancho disponible para teclas

    // ── PASADA 1: Identificar filas y sumar pesos por fila ──
    int row_start[10];  // Índice de inicio de cada fila
    int row_count[10];  // Número de teclas por fila
    int row_weight[10]; // Suma de pesos por fila
    int num_rows = 0;

    for (int i = 0; i < state->layout->num_keys; i++) {
        KeyDef *k = &state->layout->keys[i];
        if (k->new_row || i == 0) {
            row_start[num_rows] = i;
            row_count[num_rows] = 0;
            row_weight[num_rows] = 0;
            num_rows++;
        }
        row_count[num_rows - 1]++;
        row_weight[num_rows - 1] += k->width_weight;
    }

    // ── PASADA 2: Dibujar cada fila escalada al 100% del ancho ──
    state->num_rects = 0;
    int cur_y = pad;

    for (int r = 0; r < num_rows; r++) {
        int n = row_count[r];
        int total_gap = gap * (n - 1);  // Espacio total entre teclas en esta fila
        int usable_w = total_w - total_gap;  // Ancho real para las teclas

        int cur_x = pad;

        for (int j = 0; j < n; j++) {
            int idx = row_start[r] + j;
            KeyDef *k = &state->layout->keys[idx];

            // Ancho proporcional: peso de esta tecla / peso total de la fila * ancho disponible
            int kw = (k->width_weight * usable_w) / row_weight[r];

            // Guardar rectángulo para hit-testing
            state->rects[state->num_rects].x = cur_x;
            state->rects[state->num_rects].y = cur_y;
            state->rects[state->num_rects].w = kw;
            state->rects[state->num_rects].h = row_h;
            state->rects[state->num_rects].key = k;

            cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

            // Cuerpo de la tecla (estilo chiclet plano)
            cairo_set_source_rgba(cr, 0.24, 0.24, 0.24, state->theme.key_opacity);
            draw_rounded_rect(cr, cur_x, cur_y, kw, row_h, state->theme.key_radius);
            cairo_fill(cr);

            // Texto centrado
            cairo_set_source_rgb(cr, 0.95, 0.95, 0.95);
            cairo_select_font_face(cr, state->theme.font_family,
                                   CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
            cairo_set_font_size(cr, state->theme.font_size);

            cairo_text_extents_t ext;
            cairo_text_extents(cr, k->label, &ext);
            cairo_move_to(cr,
                          cur_x + kw / 2.0 - ext.width / 2.0 - ext.x_bearing,
                          cur_y + row_h / 2.0 - ext.height / 2.0 - ext.y_bearing);
            cairo_show_text(cr, k->label);

            cur_x += kw + gap;
            state->num_rects++;
        }

        cur_y += row_h + gap;
    }

    cairo_destroy(cr);
    cairo_surface_destroy(surf);
}

void ui_loop(UIState *state) {
    XEvent ev;
    int drag_x = 0, drag_y = 0;
    bool dragging = false;

    while (1) {
        XNextEvent(state->display, &ev);

        if (ev.type == Expose) render(state);

        if (ev.type == ButtonPress && ev.xbutton.button == 1) {
            bool on_key = false;
            for (int i = 0; i < state->num_rects; i++) {
                KeyRect *r = &state->rects[i];
                if (ev.xbutton.x >= r->x && ev.xbutton.x <= r->x + r->w &&
                    ev.xbutton.y >= r->y && ev.xbutton.y <= r->y + r->h) {
                    engine_send_key(r->key->normal, true);
                    engine_send_key(r->key->normal, false);
                    on_key = true;
                    break;
                }
            }
            if (!on_key) {
                dragging = true;
                drag_x = ev.xbutton.x;
                drag_y = ev.xbutton.y;
            }
        }

        if (ev.type == ButtonRelease && ev.xbutton.button == 1)
            dragging = false;

        if (ev.type == MotionNotify && dragging)
            XMoveWindow(state->display, state->window,
                        ev.xmotion.x_root - drag_x, ev.xmotion.y_root - drag_y);
    }
}
