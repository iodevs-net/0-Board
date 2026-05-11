// SPDX-License-Identifier: MIT — see LICENSE file

#include "keysym_util.h"

const ShiftPair g_shift_pairs[SHIFT_PAIRS_COUNT] = {
    {XK_exclam,     XK_1},
    {XK_at,         XK_2},
    {XK_numbersign, XK_3},
    {XK_dollar,     XK_4},
    {XK_percent,    XK_5},
    {XK_asciicircum,XK_6},
    {XK_ampersand,  XK_7},
    {XK_asterisk,   XK_8},
    {XK_parenleft,  XK_9},
    {XK_parenright, XK_0},
    {XK_underscore, XK_minus},
    {XK_plus,       XK_equal},
    {XK_braceleft,  XK_bracketleft},
    {XK_braceright, XK_bracketright},
    {XK_bar,        XK_backslash},
    {XK_colon,      XK_semicolon},
    {XK_quotedbl,   XK_apostrophe},
    {XK_less,       XK_comma},
    {XK_greater,    XK_period},
    {XK_question,   XK_slash},
    {XK_asciitilde, XK_grave},
};

int keysym_is_letter(KeySym sym) {
    return (sym >= XK_a && sym <= XK_z) ||
           (sym >= XK_A && sym <= XK_Z) ||
           (sym == XK_ntilde) || (sym == XK_Ntilde);
}

KeySym keysym_get_base(KeySym sym) {
    for (int i = 0; i < SHIFT_PAIRS_COUNT; i++) {
        if (g_shift_pairs[i].shifted == sym)
            return g_shift_pairs[i].base;
    }
    return 0;
}
