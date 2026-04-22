#include "layout.h"
#include <X11/keysym.h>
#include <string.h>

// Apple Magic Keyboard layout — optimized for programmers
// Row weights tuned for visual balance and touch targets
static KeyDef default_keys[] = {
    // Row 1: Number row + backtick + backspace
    {XK_grave,     XK_asciitilde, 0, "`",  "~",  true,  6, 0},
    {XK_1,         XK_exclam,     0, "1",  "!",  false, 7, 0},
    {XK_2,         XK_at,         0, "2",  "@",  false, 7, 0},
    {XK_3,         XK_numbersign, 0, "3",  "#",  false, 7, 0},
    {XK_4,         XK_dollar,     0, "4",  "$",  false, 7, 0},
    {XK_5,         XK_percent,    0, "5",  "%",  false, 7, 0},
    {XK_6,         XK_asciicircum,0, "6",  "^",  false, 7, 0},
    {XK_7,         XK_ampersand,  0, "7",  "&",  false, 7, 0},
    {XK_8,         XK_asterisk,   0, "8",  "*",  false, 7, 0},
    {XK_9,         XK_parenleft,  0, "9",  "(",  false, 7, 0},
    {XK_0,         XK_parenright, 0, "0",  ")",  false, 7, 0},
    {XK_minus,     XK_underscore, 0, "-",  "_",  false, 7, 0},
    {XK_equal,     XK_plus,       0, "=",  "+",  false, 7, 0},
    {XK_BackSpace, 0,             0, "⌫", NULL, false, 11, 0},

    // Row 2: Tab + QWERTY + brackets + backslash
    {XK_Tab,       0,             0, "⇥",  NULL, true,  11, 0},
    {XK_q,         XK_Q,          0, "q",  "Q",  false, 7, 0},
    {XK_w,         XK_W,          0, "w",  "W",  false, 7, 0},
    {XK_e,         XK_E,          0, "e",  "E",  false, 7, 0},
    {XK_r,         XK_R,          0, "r",  "R",  false, 7, 0},
    {XK_t,         XK_T,          0, "t",  "T",  false, 7, 0},
    {XK_y,         XK_Y,          0, "y",  "Y",  false, 7, 0},
    {XK_u,         XK_U,          0, "u",  "U",  false, 7, 0},
    {XK_i,         XK_I,          0, "i",  "I",  false, 7, 0},
    {XK_o,         XK_O,          0, "o",  "O",  false, 7, 0},
    {XK_p,         XK_P,          0, "p",  "P",  false, 7, 0},
    {XK_bracketleft,  XK_braceleft,  0, "[",  "{",  false, 7, 0},
    {XK_bracketright, XK_braceright, 0, "]",  "}",  false, 7, 0},
    {XK_backslash, XK_bar,        0, "\\", "|",  false, 7, 0},

    // Row 3: Caps + ASDF + semicolon + quote + return
    {XK_Caps_Lock, 0,             0, "⇪",  NULL, true,  13, 0},
    {XK_a,         XK_A,          0, "a",  "A",  false, 7, 0},
    {XK_s,         XK_S,          0, "s",  "S",  false, 7, 0},
    {XK_d,         XK_D,          0, "d",  "D",  false, 7, 0},
    {XK_f,         XK_F,          0, "f",  "F",  false, 7, 0},
    {XK_g,         XK_G,          0, "g",  "G",  false, 7, 0},
    {XK_h,         XK_H,          0, "h",  "H",  false, 7, 0},
    {XK_j,         XK_J,          0, "j",  "J",  false, 7, 0},
    {XK_k,         XK_K,          0, "k",  "K",  false, 7, 0},
    {XK_l,         XK_L,          0, "l",  "L",  false, 7, 0},
    {XK_semicolon, XK_colon,      0, ";",  ":",  false, 7, 0},
    {XK_apostrophe,XK_quotedbl,   0, "'",  "\"", false, 7, 0},
    {XK_Return,    0,             0, "⏎",  NULL, false, 14, 0},

    // Row 4: Shift + ZXCV + comma/period/slash + shift
    {XK_Shift_L,   0,             0, "⇧",  NULL, true,  16, 0},
    {XK_z,         XK_Z,          0, "z",  "Z",  false, 7, 0},
    {XK_x,         XK_X,          0, "x",  "X",  false, 7, 0},
    {XK_c,         XK_C,          0, "c",  "C",  false, 7, 0},
    {XK_v,         XK_V,          0, "v",  "V",  false, 7, 0},
    {XK_b,         XK_B,          0, "b",  "B",  false, 7, 0},
    {XK_n,         XK_N,          0, "n",  "N",  false, 7, 0},
    {XK_m,         XK_M,          0, "m",  "M",  false, 7, 0},
    {XK_comma,     XK_less,       0, ",",  "<",  false, 7, 0},
    {XK_period,    XK_greater,    0, ".",  ">",  false, 7, 0},
    {XK_slash,     XK_question,   0, "/",  "?",  false, 7, 0},
    {XK_VoidSymbol, 0,             0, "↑↓",  NULL, false, 16, 0},

    // Row 5: Bottom — fn, ctrl, alt, cmd, space, cmd, alt, arrows
    {0,            0,             0, "fn",   NULL, true,  8, 0},
    {XK_Control_L, 0,             0, "ctrl", NULL, false, 8, 0},
    {XK_Alt_L,     0,             0, "alt",  NULL, false, 8, 0},
    {XK_Super_L,   0,             0, "⌘",   NULL, false, 8, 0},
    {XK_space,     0,             0, " ",    NULL, false, 36, 0},
    {XK_Super_R,   0,             0, "size", NULL, false, 8, 0},
    {XK_Alt_R,     0,             0, "alt",  NULL, false, 8, 0},
    {XK_Left,      0,             0, "←",   NULL, false, 7, 0},
    {XK_Up,        0,             0, "↑",   NULL, false, 7, 0},
    {XK_Down,      0,             0, "↓",   NULL, false, 7, 0},
    {XK_Right,     0,             0, "→",   NULL, false, 7, 0},
};

static Layout main_layout = {
    "Magic Programmer",
    default_keys,
    sizeof(default_keys) / sizeof(default_keys[0])
};

Layout* layout_get_default() { return &main_layout; }

void layout_init(Layout *l) {
    for (int i = 0; i < l->num_keys; i++) {
        KeyDef *k = &l->keys[i];
        k->flags = KEYFLAG_NORMAL;
        if (!k->label) continue;

        // Classify keys by function
        if (strcmp(k->label, "⇧") == 0)
            k->flags = KEYFLAG_SHIFT | KEYFLAG_MODIFIER;
        else if (strcmp(k->label, "fn") == 0 || strcmp(k->label, "ctrl") == 0 ||
                 strcmp(k->label, "alt") == 0 || strcmp(k->label, "⌘") == 0 ||
                 strcmp(k->label, "size") == 0 || strcmp(k->label, "⇥") == 0 ||
                 strcmp(k->label, "⌫") == 0 || strcmp(k->label, "⏎") == 0 ||
                 strcmp(k->label, "←") == 0 || strcmp(k->label, "↑") == 0 ||
                 strcmp(k->label, "↓") == 0 || strcmp(k->label, "→") == 0 ||
                 strcmp(k->label, "↑↓") == 0)
            k->flags = KEYFLAG_MODIFIER;
    }
}
