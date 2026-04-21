#include "layout.h"
#include <X11/keysym.h>

// Mapeo: {Etiqueta, Normal, Shifted, Símbolo/AltGr, Peso, NuevaFila}
KeyDef apple_keys[] = {
    // Fila 0 (Números / Símbolos)
    {"1", XK_1, XK_exclam, XK_bar, 100, false},
    {"2", XK_2, XK_quotedbl, XK_at, 100, false},
    {"3", XK_3, XK_periodcentered, XK_numbersign, 100, false},
    {"4", XK_4, XK_dollar, XK_asciitilde, 100, false},
    {"5", XK_5, XK_percent, XK_bracketleft, 100, false},
    {"6", XK_6, XK_ampersand, XK_bracketright, 100, false},
    {"7", XK_7, XK_slash, XK_braceleft, 100, false},
    {"8", XK_8, XK_parenleft, XK_braceright, 100, false},
    {"9", XK_9, XK_parenright, XK_backslash, 100, false},
    {"0", XK_0, XK_equal, XK_question, 100, false},
    {"del", XK_BackSpace, XK_BackSpace, XK_BackSpace, 150, false},

    // Fila 1 (QWERTY)
    {"tab", XK_Tab, XK_Tab, XK_Tab, 155, true},
    {"q", XK_q, XK_Q, XK_less, 100, false},
    {"w", XK_w, XK_W, XK_greater, 100, false},
    {"e", XK_e, XK_E, XK_equal, 100, false},
    {"r", XK_r, XK_R, XK_plus, 100, false},
    {"t", XK_t, XK_T, XK_asterisk, 100, false},
    {"y", XK_y, XK_Y, XK_percent, 100, false},
    {"u", XK_u, XK_U, XK_underscore, 100, false},
    {"i", XK_i, XK_I, XK_minus, 100, false},
    {"o", XK_o, XK_O, XK_colon, 100, false},
    {"p", XK_p, XK_P, XK_semicolon, 100, false},

    // Fila 2 (ASDF)
    {"caps", XK_Caps_Lock, XK_Caps_Lock, XK_Caps_Lock, 185, true},
    {"a", XK_a, XK_A, XK_bracketleft, 100, false},
    {"s", XK_s, XK_S, XK_bracketright, 100, false},
    {"d", XK_d, XK_D, XK_braceleft, 100, false},
    {"f", XK_f, XK_F, XK_braceright, 100, false},
    {"g", XK_g, XK_G, XK_grave, 100, false},
    {"h", XK_h, XK_H, XK_asciitilde, 100, false},
    {"j", XK_j, XK_J, XK_bar, 100, false},
    {"k", XK_k, XK_K, XK_backslash, 100, false},
    {"l", XK_l, XK_L, XK_apostrophe, 100, false},
    {"ñ", 0xf1, 0xd1, XK_dead_tilde, 100, false},
    {"return", XK_Return, XK_Return, XK_Return, 185, false},

    // Fila 3 (ZXCV / Símbolos de Programación)
    {"shift", XK_Shift_L, XK_Shift_L, XK_Shift_L, 245, true},
    {"z", XK_z, XK_Z, XK_slash, 100, false},
    {"x", XK_x, XK_X, XK_question, 100, false},
    {"c", XK_c, XK_C, XK_exclam, 100, false},
    {"v", XK_v, XK_V, XK_quotedbl, 100, false},
    {"b", XK_b, XK_B, XK_apostrophe, 100, false},
    {"n", XK_n, XK_N, XK_comma, 100, false},
    {"m", XK_m, XK_M, XK_period, 100, false},
    {",", XK_comma, XK_semicolon, XK_less, 100, false},
    {".", XK_period, XK_colon, XK_greater, 100, false},
    {"shift", XK_Shift_R, XK_Shift_R, XK_Shift_R, 245, false},

    // Fila 4 (Modificadores + Tecla Símbolos)
    {"?123", XK_VoidSymbol, XK_VoidSymbol, XK_VoidSymbol, 135, true},
    {"opt", XK_Alt_L, 0, 0, 135, false},
    {"cmd", XK_Super_L, 0, 0, 160, false},
    {" ", XK_space, XK_space, XK_space, 550, false},
    {"cmd", XK_Super_R, 0, 0, 160, false},
    {"opt", XK_Alt_R, 0, 0, 135, false}
};

Layout apple_layout = {
    "Apple Programmer",
    apple_keys,
    sizeof(apple_keys) / sizeof(KeyDef)
};
