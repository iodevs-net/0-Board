// SPDX-License-Identifier: MIT — see LICENSE file
//
// keyboard.c — Fachada pública sobre KeyboardState.
// keyboard_state.h se ha fusionado en keyboard.h; KbdState eliminado.

#include "keyboard.h"
#include <X11/keysym.h>
#include <stdlib.h>
#include <string.h>

struct Keyboard {
    Layout        *layout;
    KeyboardState  state;   /* estado único, sin copias */
    bool           owns_layout;
};

Keyboard* keyboard_create(Layout *layout) {
    Keyboard *kb = calloc(1, sizeof(Keyboard));
    if (!kb) return NULL;

    kb->layout = layout;
    kbd_reset(&kb->state);
    return kb;
}

void keyboard_set_layout(Keyboard *kb, Layout *layout) {
    if (kb) kb->layout = layout;
}

Layout* keyboard_get_layout(const Keyboard *kb) {
    return kb ? kb->layout : NULL;
}

void keyboard_destroy(Keyboard *kb) {
    free(kb);
}

/* ---------------------------------------------------------------- */
/*  Pulsación / liberación                                           */
/* ---------------------------------------------------------------- */

void keyboard_press_key(Keyboard *kb, int key_index) {
    if (!kb || !kb->layout || key_index < 0 || key_index >= kb->layout->num_keys)
        return;

    kb->state.pressed_key_index = key_index;

    KeyDef *key = &kb->layout->keys[key_index];
    if (key->flags & KEYFLAG_SHIFT)
        kbd_toggle_shift(&kb->state);
    else if (key->flags & KEYFLAG_CTRL)
        kbd_toggle_ctrl(&kb->state);
    else if (key->flags & KEYFLAG_ALT)
        kbd_toggle_alt(&kb->state);
    else if (key->flags & KEYFLAG_META)
        kbd_toggle_meta(&kb->state);
    else if (key->flags & KEYFLAG_FN)
        kbd_toggle_fn(&kb->state);
    else if (key->normal == XK_Caps_Lock)
        kbd_toggle_caps(&kb->state);

    kb->state.dirty = true;
}

void keyboard_release_key(Keyboard *kb, int key_index) {
    if (!kb) return;
    (void)key_index;
    kb->state.pressed_key_index = -1;
    kb->state.dirty = true;
}

void keyboard_notify_key_sent(Keyboard *kb, int key_index) {
    if (!kb || !kb->layout || key_index < 0 || key_index >= kb->layout->num_keys)
        return;

    KeyDef *key = &kb->layout->keys[key_index];
    kbd_notify_key_sent(&kb->state, key);
}

/* ---------------------------------------------------------------- */
/*  Selectores de capa                                               */
/* ---------------------------------------------------------------- */

KeySym keyboard_get_keysym(const Keyboard *kb, int key_index) {
    if (!kb || !kb->layout || key_index < 0 || key_index >= kb->layout->num_keys)
        return 0;

    KeyDef *key = &kb->layout->keys[key_index];
    KeyboardLayer layer = kbd_get_effective_layer(&kb->state, key);

    if (layer == KEYBOARD_LAYER_SHIFT && key->shifted != 0) return key->shifted;
    if (layer == KEYBOARD_LAYER_ALTGR  && key->altgr  != 0) return key->altgr;
    if (layer == KEYBOARD_LAYER_FN     && key->fn_keysym != 0) return key->fn_keysym;
    return key->normal;
}

const char* keyboard_get_key_label(const Keyboard *kb, int key_index) {
    if (!kb || !kb->layout || key_index < 0 || key_index >= kb->layout->num_keys)
        return NULL;

    KeyDef *key = &kb->layout->keys[key_index];
    KeyboardLayer layer = kbd_get_effective_layer(&kb->state, key);

    if (layer == KEYBOARD_LAYER_SHIFT && key->shifted_label) return key->shifted_label;
    if (layer == KEYBOARD_LAYER_FN     && key->fn_label)     return key->fn_label;
    return key->label;
}

int keyboard_get_effective_layer(const Keyboard *kb, int key_index) {
    if (!kb || !kb->layout || key_index < 0 || key_index >= kb->layout->num_keys)
        return 0;
    KeyDef *key = &kb->layout->keys[key_index];
    return (int)kbd_get_effective_layer(&kb->state, key);
}

/* ---------------------------------------------------------------- */
/*  Modificadores                                                    */
/* ---------------------------------------------------------------- */

int keyboard_get_active_modifiers(const Keyboard *kb) {
    if (!kb) return 0;
    return kbd_get_modifier_mask(&kb->state);
}

int keyboard_get_modifiers_for_keysym(const Keyboard *kb, KeySym sym) {
    if (!kb) return 0;
    return kbd_get_modifier_mask_for_key(&kb->state, sym);
}

void keyboard_toggle_shift(Keyboard *kb)     { if (kb) kbd_toggle_shift(&kb->state); }
void keyboard_toggle_caps_lock(Keyboard *kb) { if (kb) kbd_toggle_caps(&kb->state); }
void keyboard_toggle_fn(Keyboard *kb)        { if (kb) kbd_toggle_fn(&kb->state); }

/* ---------------------------------------------------------------- */
/*  Estado                                                           */
/* ---------------------------------------------------------------- */

KeyboardState keyboard_get_state(const Keyboard *kb) {
    KeyboardState s = {0};
    if (kb) s = kb->state;
    return s;
}

bool keyboard_is_dirty(const Keyboard *kb)   { return kb ? kb->state.dirty : false; }
void keyboard_mark_clean(Keyboard *kb)       { if (kb) kb->state.dirty = false; }
