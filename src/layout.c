// SPDX-License-Identifier: MIT — see LICENSE file

#include "layout.h"
#include <X11/keysym.h>

/* Macros de acceso directo para flags compuestos comunes */
#define F_MOD  KEYFLAG_MODIFIER
#define F_SHF  (KEYFLAG_SHIFT  | KEYFLAG_MODIFIER)
#define F_CTL  (KEYFLAG_CTRL   | KEYFLAG_MODIFIER)
#define F_ALT  (KEYFLAG_ALT    | KEYFLAG_MODIFIER)
#define F_MET  (KEYFLAG_META   | KEYFLAG_MODIFIER)
#define F_FN   (KEYFLAG_FN     | KEYFLAG_MODIFIER)
#define F_TP   (KEYFLAG_TOUCHPAD | KEYFLAG_MODIFIER)

/*
 * Disposición tipo Apple Magic Keyboard — optimizada para programadores.
 * Pesos de fila calibrados para equilibrio visual y blancos táctiles.
 *
 * Cada tecla lleva sus flags precalculados en compilación:
 * ni strcmp, ni layout_init que deduzca flags desde etiquetas.
 */
static KeyDef default_keys[] = {
    /* Fila 1: Números + backtick + retroceso (Total 300) */
    {XK_grave,     XK_asciitilde, 0, XK_Escape, "esc", "`",  "~",  true,  20, 0},
    {XK_1,         XK_exclam,     0, XK_F1,     "F1",  "1",  "!",  false, 20, 0},
    {XK_2,         XK_at,         0, XK_F2,     "F2",  "2",  "@",  false, 20, 0},
    {XK_3,         XK_numbersign, 0, XK_F3,     "F3",  "3",  "#",  false, 20, 0},
    {XK_4,         XK_dollar,     0, XK_F4,     "F4",  "4",  "$",  false, 20, 0},
    {XK_5,         XK_percent,    0, XK_F5,     "F5",  "5",  "%",  false, 20, 0},
    {XK_6,         XK_asciicircum,0, XK_F6,     "F6",  "6",  "^",  false, 20, 0},
    {XK_7,         XK_ampersand,  0, XK_F7,     "F7",  "7",  "&",  false, 20, 0},
    {XK_8,         XK_asterisk,   0, XK_F8,     "F8",  "8",  "*",  false, 20, 0},
    {XK_9,         XK_parenleft,  0, XK_F9,     "F9",  "9",  "(",  false, 20, 0},
    {XK_0,         XK_parenright, 0, XK_F10,    "F10", "0",  ")",  false, 20, 0},
    {XK_minus,     XK_underscore, 0, XK_F11,    "F11", "-",  "_",  false, 20, 0},
    {XK_equal,     XK_plus,       0, XK_F12,    "F12", "=",  "+",  false, 20, 0},
    {XK_BackSpace, 0,             0, 0, NULL,    "⌫",  NULL, false, 40, F_MOD},

    /* Fila 2: Tabulador + QWERTY + corchetes + barra (Total 300) */
    {XK_Tab,       0,             0, 0, NULL, "⇥",  NULL, true,  30, F_MOD},
    {XK_q,         XK_Q,          0, 0, NULL, "q",  "Q",  false, 20, 0},
    {XK_w,         XK_W,          0, 0, NULL, "w",  "W",  false, 20, 0},
    {XK_e,         XK_E,          0, 0, NULL, "e",  "E",  false, 20, 0},
    {XK_r,         XK_R,          0, 0, NULL, "r",  "R",  false, 20, 0},
    {XK_t,         XK_T,          0, 0, NULL, "t",  "T",  false, 20, 0},
    {XK_y,         XK_Y,          0, 0, NULL, "y",  "Y",  false, 20, 0},
    {XK_u,         XK_U,          0, 0, NULL, "u",  "U",  false, 20, 0},
    {XK_i,         XK_I,          0, 0, NULL, "i",  "I",  false, 20, 0},
    {XK_o,         XK_O,          0, 0, NULL, "o",  "O",  false, 20, 0},
    {XK_p,         XK_P,          0, 0, NULL, "p",  "P",  false, 20, 0},
    {XK_bracketleft,  XK_braceleft,  0, 0, NULL, "[",  "{",  false, 20, 0},
    {XK_bracketright, XK_braceright, 0, 0, NULL, "]",  "}",  false, 20, 0},
    {XK_backslash, XK_bar,        0, 0, NULL, "\\", "|",  false, 30, 0},

    /* Fila 3: BloqMayús + ASDF + punto y coma + comilla + intro (Total 300) */
    {XK_Caps_Lock, 0,             0, 0, NULL, "⇪",  NULL, true,  35, F_MOD},
    {XK_a,         XK_A,          0, 0, NULL, "a",  "A",  false, 20, 0},
    {XK_s,         XK_S,          0, 0, NULL, "s",  "S",  false, 20, 0},
    {XK_d,         XK_D,          0, 0, NULL, "d",  "D",  false, 20, 0},
    {XK_f,         XK_F,          0, 0, NULL, "f",  "F",  false, 20, 0},
    {XK_g,         XK_G,          0, 0, NULL, "g",  "G",  false, 20, 0},
    {XK_h,         XK_H,          0, 0, NULL, "h",  "H",  false, 20, 0},
    {XK_j,         XK_J,          0, 0, NULL, "j",  "J",  false, 20, 0},
    {XK_k,         XK_K,          0, 0, NULL, "k",  "K",  false, 20, 0},
    {XK_l,         XK_L,          0, 0, NULL, "l",  "L",  false, 20, 0},
    {XK_ntilde,    XK_Ntilde,     0, 0, NULL, "ñ",  "Ñ",  false, 20, 0},
    {XK_semicolon, XK_colon,      0, 0, NULL, ";",  ":",  false, 20, 0},
    {XK_apostrophe,XK_quotedbl,   0, 0, NULL, "'",  "\"", false, 20, 0},
    {XK_Return,    0,             0, 0, NULL, "⏎",  NULL, false, 25, F_MOD},

    /* Fila 4: Mayús + ZXCV + coma/punto/barra + Mayús extra (Total 300) */
    {XK_Shift_L,   0,             0, 0, NULL, "⇧",  NULL, true,  45, F_SHF},
    {XK_z,         XK_Z,          0, 0, NULL, "z",  "Z",  false, 20, 0},
    {XK_x,         XK_X,          0, 0, NULL, "x",  "X",  false, 20, 0},
    {XK_c,         XK_C,          0, 0, NULL, "c",  "C",  false, 20, 0},
    {XK_v,         XK_V,          0, 0, NULL, "v",  "V",  false, 20, 0},
    {XK_b,         XK_B,          0, 0, NULL, "b",  "B",  false, 20, 0},
    {XK_n,         XK_N,          0, 0, NULL, "n",  "N",  false, 20, 0},
    {XK_m,         XK_M,          0, 0, NULL, "m",  "M",  false, 20, 0},
    {XK_comma,     XK_less,       0, 0, NULL, ",",  "<",  false, 20, 0},
    {XK_period,    XK_greater,    0, 0, NULL, ".",  ">",  false, 20, 0},
    {XK_slash,     XK_question,   0, 0, NULL, "/",  "?",  false, 20, 0},
    {XK_VoidSymbol, 0,            0, 0, NULL, "⦿",  NULL, false, 20, F_TP},
    {XK_VoidSymbol, 0,            0, 0, NULL, "↑↓", NULL, false, 25, F_MOD},

    /* Fila 5: fn, ctrl, alt, mic, espacio, size, alt, flechas (Total 300) */
    {0,            0,             0, 0, NULL, "fn",   NULL, true,  20, F_FN},
    {XK_Control_L, 0,             0, 0, NULL, "ctrl", NULL, false, 20, F_CTL},
    {XK_Alt_L,     0,             0, 0, NULL, "alt",  NULL, false, 20, F_ALT},
    {XK_Super_L,   0,             0, 0, NULL, "mic",  NULL, false, 25, F_MET},
    {XK_space,     0,             0, 0, NULL, " ",    NULL, false, 90, 0},
    {XK_Super_R,   0,             0, 0, NULL, "size", NULL, false, 25, F_MOD},
    {XK_Alt_R,     0,             0, 0, NULL, "alt",  NULL, false, 20, F_ALT},
    {XK_Left,      0,             0, 0, NULL, "←",   NULL, false, 20, F_MOD},
    {XK_Up,        0,             0, 0, NULL, "↑",   NULL, false, 20, F_MOD},
    {XK_Down,      0,             0, 0, NULL, "↓",   NULL, false, 20, F_MOD},
    {XK_Right,     0,             0, 0, NULL, "→",   NULL, false, 20, F_MOD},
};

static Layout main_layout = {
    "Magic Programmer",
    default_keys,
    sizeof(default_keys) / sizeof(default_keys[0])
};

Layout* layout_get_default(void) {
    return &main_layout;
}

void layout_init(Layout *l) {
    /* Todos los flags ya están predefinidos en el inicializador.
     * Esta función queda como punto de extensión para layouts
     * generados dinámicamente que necesiten post-procesado. */
    (void)l;
}
