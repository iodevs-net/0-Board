// SPDX-License-Identifier: MIT — see LICENSE file
//
// keyboard_state.c — Máquina de estados de modificadores (one-shot / locked).
// Tipos unificados en keyboard.h (KeyboardState). keyboard_state.h eliminado.

#include "keyboard.h"
#include <X11/keysym.h>
#include "debug.h"
#include "keysym_util.h"

/* ---------------------------------------------------------------- */
/*  Interno: ciclo OFF → ONESHOT → LOCKED → OFF                     */
/* ---------------------------------------------------------------- */

static void toggle_modifier(KbdModifierState *ms) {
    if (*ms == KBD_MODIFIER_OFF) {
        *ms = KBD_MODIFIER_ONESHOT;
    } else if (*ms == KBD_MODIFIER_ONESHOT) {
        *ms = KBD_MODIFIER_LOCKED;
    } else {
        *ms = KBD_MODIFIER_OFF;
    }
}

/* ---------------------------------------------------------------- */
/*  API pública de la máquina de estados                             */
/* ---------------------------------------------------------------- */

void kbd_reset(KeyboardState *st) {
    if (!st) return;
    st->active_layer       = KEYBOARD_LAYER_NORMAL;
    st->shift_locked       = false;
    st->ctrl_state         = KBD_MODIFIER_OFF;
    st->alt_state          = KBD_MODIFIER_OFF;
    st->meta_state         = KBD_MODIFIER_OFF;
    st->fn_state           = KBD_MODIFIER_OFF;
    st->caps_lock          = false;
    st->pressed_key_index  = -1;
    st->dirty              = true;
}

bool kbd_is_letter(KeySym sym) {
    return keysym_is_letter(sym);
}

KeyboardLayer kbd_get_effective_layer(const KeyboardState *st, KeyDef *key) {
    if (!st || !key) return KEYBOARD_LAYER_NORMAL;

    if (st->active_layer == KEYBOARD_LAYER_SHIFT)
        return KEYBOARD_LAYER_SHIFT;

    if (st->fn_state != KBD_MODIFIER_OFF)
        return KEYBOARD_LAYER_FN;

    if (st->caps_lock && keysym_is_letter(key->normal))
        return KEYBOARD_LAYER_SHIFT;

    return st->active_layer;
}

void kbd_toggle_shift(KeyboardState *st) {
    if (!st) return;

    if (st->active_layer == KEYBOARD_LAYER_NORMAL) {
        st->active_layer = KEYBOARD_LAYER_SHIFT;
        st->shift_locked = false;
        LOG_DEBUG("State: Shift One-Shot active");
    } else if (st->active_layer == KEYBOARD_LAYER_SHIFT && !st->shift_locked) {
        st->shift_locked = true;
        LOG_DEBUG("State: Shift LOCKED active");
    } else {
        st->active_layer = KEYBOARD_LAYER_NORMAL;
        st->shift_locked = false;
        LOG_DEBUG("State: Shift OFF");
    }
    st->dirty = true;
}

void kbd_toggle_ctrl(KeyboardState *st)  { if (st) { toggle_modifier(&st->ctrl_state); st->dirty = true; } }
void kbd_toggle_alt(KeyboardState *st)   { if (st) { toggle_modifier(&st->alt_state);  st->dirty = true; } }
void kbd_toggle_meta(KeyboardState *st)  { if (st) { toggle_modifier(&st->meta_state); st->dirty = true; } }
void kbd_toggle_fn(KeyboardState *st)    { if (st) { toggle_modifier(&st->fn_state);   st->dirty = true; } }

void kbd_toggle_caps(KeyboardState *st) {
    if (!st) return;
    st->caps_lock = !st->caps_lock;
    st->dirty = true;
    LOG_DEBUG("State: CapsLock toggled to %s",
              st->caps_lock ? "ON" : "OFF");
}

void kbd_notify_key_sent(KeyboardState *st, KeyDef *key) {
    if (!st || !key) return;

    /* Shift One-Shot: consumir si la tecla enviada no es Shift */
    if (st->active_layer == KEYBOARD_LAYER_SHIFT && !st->shift_locked) {
        if (!(key->flags & KEYFLAG_SHIFT)) {
            st->active_layer = KEYBOARD_LAYER_NORMAL;
            st->dirty = true;
            LOG_DEBUG("State: One-Shot Shift consumed by key");
        }
    }

    /* Ctrl / Alt / Meta / FN One-Shot */
    if (st->ctrl_state == KBD_MODIFIER_ONESHOT && !(key->flags & KEYFLAG_CTRL))
        { st->ctrl_state = KBD_MODIFIER_OFF; st->dirty = true; }
    if (st->alt_state == KBD_MODIFIER_ONESHOT && !(key->flags & KEYFLAG_ALT))
        { st->alt_state = KBD_MODIFIER_OFF; st->dirty = true; }
    if (st->meta_state == KBD_MODIFIER_ONESHOT && !(key->flags & KEYFLAG_META))
        { st->meta_state = KBD_MODIFIER_OFF; st->dirty = true; }
    if (st->fn_state == KBD_MODIFIER_ONESHOT && !(key->flags & KEYFLAG_FN))
        { st->fn_state = KBD_MODIFIER_OFF; st->dirty = true;
          LOG_DEBUG("State: One-Shot FN consumed by key"); }
}

int kbd_get_modifier_mask(const KeyboardState *st) {
    if (!st) return 0;
    int mask = 0;
    if (st->active_layer == KEYBOARD_LAYER_SHIFT || st->shift_locked) mask |= 1;
    if (st->ctrl_state != KBD_MODIFIER_OFF)                       mask |= 2;
    if (st->alt_state != KBD_MODIFIER_OFF)                        mask |= 4;
    if (st->meta_state != KBD_MODIFIER_OFF)                       mask |= 8;
    if (st->fn_state != KBD_MODIFIER_OFF)                         mask |= 16;
    return mask;
}

int kbd_get_modifier_mask_for_key(const KeyboardState *st, KeySym keysym) {
    if (!st) return 0;
    int mask = kbd_get_modifier_mask(st);
    if (st->caps_lock && keysym_is_letter(keysym))
        mask |= 1;   /* incluir Shift para mayúsculas */
    return mask;
}
