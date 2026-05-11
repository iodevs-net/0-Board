// SPDX-License-Identifier: MIT — see LICENSE file

#include "keyboard_state.h"
#include <X11/keysym.h>
#include "debug.h"
#include "keysym_util.h"

bool kbd_state_is_letter(KeySym sym) {
    return keysym_is_letter(sym);
}

Layer kbd_state_get_effective_layer(const KbdState *state, KeyDef *key) {
    if (!state || !key) return LAYER_NORMAL;

    // Si Shift está activo, SIEMPRE usamos la capa Shift (Layer 1)
    if (state->active_layer == LAYER_SHIFT) return LAYER_SHIFT;

    // Si FN está activo, usamos la capa FN
    if (state->fn_active) return LAYER_FN;
 
    // Si CapsLock está activo, solo usamos la capa Shift para letras
    if (state->caps_lock && keysym_is_letter(key->normal)) return LAYER_SHIFT;

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

static void toggle_modifier(ModifierState *ms, const char *name) {
    if (*ms == MODIFIER_OFF) {
        *ms = MODIFIER_ONESHOT;
        LOG_DEBUG("State: %s One-Shot active", name);
    } else if (*ms == MODIFIER_ONESHOT) {
        *ms = MODIFIER_LOCKED;
        LOG_DEBUG("State: %s LOCKED active", name);
    } else {
        *ms = MODIFIER_OFF;
        LOG_DEBUG("State: %s OFF", name);
    }
}

void kbd_state_toggle_ctrl(KbdState *state) {
    if (!state) return;
    toggle_modifier(&state->ctrl_state, "Ctrl");
    state->dirty = true;
}

void kbd_state_toggle_alt(KbdState *state) {
    if (!state) return;
    toggle_modifier(&state->alt_state, "Alt");
    state->dirty = true;
}

void kbd_state_toggle_meta(KbdState *state) {
    if (!state) return;
    toggle_modifier(&state->meta_state, "Meta");
    state->dirty = true;
}
void kbd_state_toggle_fn(KbdState *state) {
    if (!state) return;
    if (!state->fn_active) {
        state->fn_active = true;
        state->fn_locked = false;
        LOG_DEBUG("State: FN One-Shot active");
    } else if (state->fn_active && !state->fn_locked) {
        state->fn_locked = true;
        LOG_DEBUG("State: FN LOCKED active");
    } else {
        state->fn_active = false;
        state->fn_locked = false;
        LOG_DEBUG("State: FN OFF");
    }
    state->dirty = true;
}

void kbd_state_notify_key_sent(KbdState *state, KeyDef *key) {
    if (!state || !key) return;

    // Shift One-Shot logic
    if (state->active_layer == LAYER_SHIFT && !state->shift_locked) {
        if (!(key->flags & KEYFLAG_SHIFT)) {
            state->active_layer = LAYER_NORMAL;
            state->dirty = true;
            LOG_DEBUG("State: One-Shot Shift consumed by key");
        }
    }

    // Ctrl/Alt/Meta One-Shot logic
    if (state->ctrl_state == MODIFIER_ONESHOT && !(key->flags & KEYFLAG_CTRL)) {
        state->ctrl_state = MODIFIER_OFF;
        state->dirty = true;
    }
    if (state->alt_state == MODIFIER_ONESHOT && !(key->flags & KEYFLAG_ALT)) {
        state->alt_state = MODIFIER_OFF;
        state->dirty = true;
    }
    if (state->meta_state == MODIFIER_ONESHOT && !(key->flags & KEYFLAG_META)) {
        state->meta_state = MODIFIER_OFF;
        state->dirty = true;
    }
    // FN One-Shot logic
    if (state->fn_active && !state->fn_locked && !(key->flags & KEYFLAG_FN)) {
        state->fn_active = false;
        state->dirty = true;
        LOG_DEBUG("State: One-Shot FN consumed by key");
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
    state->ctrl_state = MODIFIER_OFF;
    state->alt_state = MODIFIER_OFF;
    state->meta_state = MODIFIER_OFF;
    state->fn_active = false;
    state->fn_locked = false;
    state->caps_lock = false;
    state->pressed_key_index = -1;
    state->dirty = true;
}

int kbd_state_get_modifier_mask(const KbdState *state) {
    if (!state) return 0;
    int mask = 0;
    if (state->active_layer == LAYER_SHIFT || state->shift_locked) mask |= 1; // Shift
    if (state->ctrl_state != MODIFIER_OFF) mask |= 2; // Ctrl
    if (state->alt_state != MODIFIER_OFF) mask |= 4; // Alt
    if (state->meta_state != MODIFIER_OFF) mask |= 8; // Meta
    if (state->fn_active) mask |= 16; // FN
    return mask;
}

int kbd_state_get_modifier_mask_for_key(const KbdState *state, KeySym keysym) {
    if (!state) return 0;
    int mask = kbd_state_get_modifier_mask(state);
    // Caps Lock: produce uppercase via Shift only for letter keys
    if (state->caps_lock && keysym_is_letter(keysym)) {
        mask |= 1; // Include Shift
    }
    return mask;
}
