#ifndef KEYBOARD_STATE_H
#define KEYBOARD_STATE_H

#include <stdbool.h>
#include "layout.h"

typedef enum {
    LAYER_NORMAL = 0,
    LAYER_SHIFT  = 1,
    LAYER_ALTGR  = 2
} Layer;

typedef struct {
    Layer active_layer;
    bool shift_locked;
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
void kbd_state_toggle_caps(KbdState *state);
void kbd_state_reset(KbdState *state);

#endif
