// SPDX-License-Identifier: MIT — see LICENSE file

#ifndef KEYBOARD_STATE_H
#define KEYBOARD_STATE_H

#include <stdbool.h>
#include "layout.h"

typedef enum {
    LAYER_NORMAL = 0,
    LAYER_SHIFT  = 1,
    LAYER_ALTGR  = 2
} Layer;

typedef enum {
    MODIFIER_OFF = 0,
    MODIFIER_ONESHOT = 1,
    MODIFIER_LOCKED = 2
} ModifierState;

typedef struct {
    Layer active_layer;
    bool shift_locked;
    ModifierState ctrl_state;
    ModifierState alt_state;
    ModifierState meta_state;
    bool caps_lock;
    int pressed_key_index; // -1 if none
    bool dirty;
} KbdState;

// Se llama cuando se envía una tecla al sistema
void kbd_state_notify_key_sent(KbdState *state, KeyDef *key);

// Determina qué capa debe usarse para una tecla específica
// considerando si es una letra (afectada por CapsLock) o un símbolo.
Layer kbd_state_get_effective_layer(const KbdState *state, KeyDef *key);

// Acciones de estado
void kbd_state_toggle_shift(KbdState *state);
void kbd_state_toggle_ctrl(KbdState *state);
void kbd_state_toggle_alt(KbdState *state);
void kbd_state_toggle_meta(KbdState *state);
void kbd_state_toggle_caps(KbdState *state);
void kbd_state_reset(KbdState *state);

int kbd_state_get_modifier_mask(const KbdState *state);
int kbd_state_get_modifier_mask_for_key(const KbdState *state, KeySym keysym);

// Check if a keysym is a letter (A-Z/a-z), used by Caps Lock logic
bool kbd_state_is_letter(KeySym sym);

#endif
