#include "keyboard_state.h"
#include <X11/keysym.h>
#include "debug.h"

static bool is_letter(KeySym sym) {
    return (sym >= XK_a && sym <= XK_z) || 
           (sym >= XK_A && sym <= XK_Z) ||
           (sym == 0xf1) || (sym == 0xd1); // ñ / Ñ
}

Layer kbd_state_get_effective_layer(const KbdState *state, KeyDef *key) {
    if (!state || !key) return LAYER_NORMAL;

    // Si Shift está activo, SIEMPRE usamos la capa Shift (Layer 1)
    if (state->active_layer == LAYER_SHIFT) return LAYER_SHIFT;

    // Si CapsLock está activo, solo usamos la capa Shift para letras
    if (state->caps_lock && is_letter(key->normal)) return LAYER_SHIFT;

    return state->active_layer;
}

void kbd_state_toggle_shift(KbdState *state) {
    if (!state) return;

    if (state->active_layer == LAYER_NORMAL) {
        state->active_layer = LAYER_SHIFT;
        state->shift_locked = false; // Modo One-Shot
        LOG_DEBUG("State: Shift One-Shot active");
    } else if (state->active_layer == LAYER_SHIFT && !state->shift_locked) {
        state->shift_locked = true; // Modo Bloqueado
        LOG_DEBUG("State: Shift LOCKED active");
    } else {
        state->active_layer = LAYER_NORMAL;
        state->shift_locked = false;
        LOG_DEBUG("State: Shift OFF");
    }
    state->dirty = true;
}

void kbd_state_notify_key_sent(KbdState *state, KeyDef *key) {
    if (!state || !key) return;

    // Si estamos en modo One-Shot y pulsamos una tecla normal
    if (state->active_layer == LAYER_SHIFT && !state->shift_locked) {
        if (!(key->flags & KEYFLAG_SHIFT)) {
            state->active_layer = LAYER_NORMAL;
            state->dirty = true;
            LOG_DEBUG("State: One-Shot Shift consumed by key");
        }
    }
}

void kbd_state_toggle_caps(KbdState *state) {
    if (!state) return;
    state->caps_lock = !state->caps_lock;
    state->dirty = true;
    LOG_DEBUG("State: CapsLock toggled to %s", state->caps_lock ? "ON" : "OFF");
}

void kbd_state_reset(KbdState *state) {
    if (!state) return;
    state->active_layer = LAYER_NORMAL;
    state->shift_locked = false;
    state->caps_lock = false;
    state->pressed_key_index = -1;
    state->dirty = true;
}
