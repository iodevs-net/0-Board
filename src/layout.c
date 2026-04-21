#include "layout.h"
#include <X11/keysym.h>

KeyDef apple_keys[] = {
    // Fila 0 (Números) - {label, normal, shifted, altgr, weight, new_row}
    {"1", XK_1, 0, 0, 100, false}, {"2", XK_2, 0, 0, 100, false}, {"3", XK_3, 0, 0, 100, false}, {"4", XK_4, 0, 0, 100, false},
    {"5", XK_5, 0, 0, 100, false}, {"6", XK_6, 0, 0, 100, false}, {"7", XK_7, 0, 0, 100, false}, {"8", XK_8, 0, 0, 100, false},
    {"9", XK_9, 0, 0, 100, false}, {"0", XK_0, 0, 0, 100, false}, {"del", XK_BackSpace, 0, 0, 150, false},

    // Fila 1 (QWERTY)
    {"tab", XK_Tab, 0, 0, 155, true}, {"q", XK_q, 0, 0, 100, false}, {"w", XK_w, 0, 0, 100, false}, {"e", XK_e, 0, 0, 100, false},
    {"r", XK_r, 0, 0, 100, false}, {"t", XK_t, 0, 0, 100, false}, {"y", XK_y, 0, 0, 100, false}, {"u", XK_u, 0, 0, 100, false},
    {"i", XK_i, 0, 0, 100, false}, {"o", XK_o, 0, 0, 100, false}, {"p", XK_p, 0, 0, 100, false},

    // Fila 2 (ASDF)
    {"caps", XK_Caps_Lock, 0, 0, 185, true}, {"a", XK_a, 0, 0, 100, false}, {"s", XK_s, 0, 0, 100, false}, {"d", XK_d, 0, 0, 100, false},
    {"f", XK_f, 0, 0, 100, false}, {"g", XK_g, 0, 0, 100, false}, {"h", XK_h, 0, 0, 100, false}, {"j", XK_j, 0, 0, 100, false},
    {"k", XK_k, 0, 0, 100, false}, {"l", XK_l, 0, 0, 100, false}, {"ñ", 0xf1, 0, 0, 100, false}, {"return", XK_Return, 0, 0, 185, false},

    // Fila 3 (ZXCV)
    {"shift", XK_Shift_L, 0, 0, 245, true}, {"z", XK_z, 0, 0, 100, false}, {"x", XK_x, 0, 0, 100, false}, {"c", XK_c, 0, 0, 100, false},
    {"v", XK_v, 0, 0, 100, false}, {"b", XK_b, 0, 0, 100, false}, {"n", XK_n, 0, 0, 100, false}, {"m", XK_m, 0, 0, 100, false},
    {",", XK_comma, 0, 0, 100, false}, {".", XK_period, 0, 0, 100, false}, {"shift", XK_Shift_R, 0, 0, 245, false},

    // Fila 4 (Modificadores)
    {"ctrl", XK_Control_L, 0, 0, 135, true}, {"opt", XK_Alt_L, 0, 0, 135, false}, {"cmd", XK_Super_L, 0, 0, 160, false},
    {" ", XK_space, 0, 0, 550, false}, {"cmd", XK_Super_R, 0, 0, 160, false}, {"opt", XK_Alt_R, 0, 0, 135, false}
};

Layout apple_layout = {
    "Apple",
    apple_keys,
    sizeof(apple_keys) / sizeof(KeyDef)
};
