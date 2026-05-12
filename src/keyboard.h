// SPDX-License-Identifier: MIT — see LICENSE file
//
// keyboard.h — Estado del teclado virtual y máquina de estados de modificadores.
// Unifica lo que antes estaba dividido entre keyboard.h y keyboard_state.h.
// El struct KeyboardState es la ÚNICA representación del estado; no hay copias.

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "layout.h"
#include <stdbool.h>
#include <X11/Xlib.h>

/* ------------------------------------------------------------------ */
/*  Capas / Modificadores                                              */
/* ------------------------------------------------------------------ */

typedef enum {
    KEYBOARD_LAYER_NORMAL = 0,
    KEYBOARD_LAYER_SHIFT  = 1,
    KEYBOARD_LAYER_ALTGR  = 2,
    KEYBOARD_LAYER_FN     = 3,
    NUM_KEYBOARD_LAYERS
} KeyboardLayer;

typedef enum {
    KBD_MODIFIER_OFF      = 0,
    KBD_MODIFIER_ONESHOT  = 1,
    KBD_MODIFIER_LOCKED   = 2
} KbdModifierState;

/* ------------------------------------------------------------------ */
/*  KeyboardState — representación única, usada internamente           */
/* ------------------------------------------------------------------ */

typedef struct {
    KeyboardLayer  active_layer;
    bool           shift_locked;
    KbdModifierState ctrl_state;
    KbdModifierState alt_state;
    KbdModifierState meta_state;
    bool           caps_lock;
    KbdModifierState fn_state;
    int            pressed_key_index;   /* -1 = ninguna */
    bool           dirty;               /* necesita redibujo */
} KeyboardState;

/* ------------------------------------------------------------------ */
/*  Keyboard (wrapper con layout + estado)                             */
/* ------------------------------------------------------------------ */

typedef struct Keyboard Keyboard;

Keyboard*    keyboard_create(Layout *layout);
void         keyboard_set_layout(Keyboard *kb, Layout *layout);
Layout*      keyboard_get_layout(const Keyboard *kb);
void         keyboard_destroy(Keyboard *kb);

void         keyboard_press_key(Keyboard *kb, int key_index);
void         keyboard_release_key(Keyboard *kb, int key_index);
void         keyboard_notify_key_sent(Keyboard *kb, int key_index);

/* Selectores de capa */
KeySym       keyboard_get_keysym(const Keyboard *kb, int key_index);
const char*  keyboard_get_key_label(const Keyboard *kb, int key_index);
int          keyboard_get_effective_layer(const Keyboard *kb, int key_index);

int          keyboard_get_active_modifiers(const Keyboard *kb);
int          keyboard_get_modifiers_for_keysym(const Keyboard *kb, KeySym sym);

/* Alternancia de modificadores */
void         keyboard_toggle_shift(Keyboard *kb);
void         keyboard_toggle_caps_lock(Keyboard *kb);
void         keyboard_toggle_fn(Keyboard *kb);

/* Estado */
KeyboardState keyboard_get_state(const Keyboard *kb);
bool         keyboard_is_dirty(const Keyboard *kb);
void         keyboard_mark_clean(Keyboard *kb);

/* --------------------------------------------------------------- */
/*  Funciones internas de la máquina de estados (keyboard_state.c)  */
/*  Expuestas para tests (test_keyboard_state.c)                    */
/* --------------------------------------------------------------- */

void         kbd_reset(KeyboardState *st);
void         kbd_toggle_shift(KeyboardState *st);
void         kbd_toggle_ctrl(KeyboardState *st);
void         kbd_toggle_alt(KeyboardState *st);
void         kbd_toggle_meta(KeyboardState *st);
void         kbd_toggle_fn(KeyboardState *st);
void         kbd_toggle_caps(KeyboardState *st);
void         kbd_notify_key_sent(KeyboardState *st, KeyDef *key);
KeyboardLayer kbd_get_effective_layer(const KeyboardState *st, KeyDef *key);
int          kbd_get_modifier_mask(const KeyboardState *st);
int          kbd_get_modifier_mask_for_key(const KeyboardState *st, KeySym sym);
bool         kbd_is_letter(KeySym sym);

#endif /* KEYBOARD_H */
