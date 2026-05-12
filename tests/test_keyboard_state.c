// SPDX-License-Identifier: MIT — see LICENSE file
//
// Test de la máquina de estados del teclado (KeyboardState).
// keyboard_state.h se fusionó en keyboard.h — los tipos y funciones
// internas (kbd_*) están disponibles directamente.

#include <stdio.h>
#include <string.h>
#include <X11/keysym.h>
#include "keyboard.h"

static int failures = 0;
static int tests = 0;

#define ASSERT(cond, msg) do { \
    tests++; \
    if (!(cond)) { \
        fprintf(stderr, "  FAIL [%d] %s\n", tests, msg); \
        failures++; \
    } else { \
        printf("  OK   [%d] %s\n", tests, msg); \
    } \
} while(0)

int main() {
    printf("=== keyboard_state tests ===\n");

    KeyboardState s;
    kbd_reset(&s);

    /* Estado inicial */
    ASSERT(s.active_layer == KEYBOARD_LAYER_NORMAL, "initial layer is NORMAL");
    ASSERT(!s.shift_locked, "initial shift not locked");
    ASSERT(s.pressed_key_index == -1, "initial no pressed key");
    ASSERT(s.dirty == true, "reset marks dirty");

    /* Shift: normal → one-shot → locked → off */
    kbd_toggle_shift(&s);
    ASSERT(s.active_layer == KEYBOARD_LAYER_SHIFT, "toggle shift: one-shot");
    ASSERT(!s.shift_locked, "toggle shift: not locked yet");

    kbd_toggle_shift(&s);
    ASSERT(s.shift_locked == true, "toggle shift: locked");
    ASSERT(s.active_layer == KEYBOARD_LAYER_SHIFT, "toggle shift: still shift layer");

    kbd_toggle_shift(&s);
    ASSERT(s.active_layer == KEYBOARD_LAYER_NORMAL, "toggle shift: off");
    ASSERT(!s.shift_locked, "toggle shift: unlocked");

    /* One-shot: key sent → auto-release */
    kbd_reset(&s);
    kbd_toggle_shift(&s);
    KeyDef non_shift_key = { .normal = XK_a, .label = "a", .flags = KEYFLAG_NORMAL };
    kbd_notify_key_sent(&s, &non_shift_key);
    ASSERT(s.active_layer == KEYBOARD_LAYER_NORMAL, "one-shot released after non-modifier key");

    /* Caps Lock */
    kbd_reset(&s);
    kbd_toggle_caps(&s);
    ASSERT(s.caps_lock == true, "caps on");
    KeyDef letter = { .normal = XK_a, .label = "a", .flags = KEYFLAG_NORMAL };
    ASSERT(kbd_get_effective_layer(&s, &letter) == KEYBOARD_LAYER_SHIFT, "caps → shift for letters");
    KeyDef number = { .normal = XK_1, .label = "1", .flags = KEYFLAG_NORMAL };
    ASSERT(kbd_get_effective_layer(&s, &number) == KEYBOARD_LAYER_NORMAL, "caps → normal for numbers");

    /* Modifier mask */
    kbd_reset(&s);
    ASSERT(kbd_get_modifier_mask(&s) == 0, "no modifiers = mask 0");
    s.ctrl_state = KBD_MODIFIER_ONESHOT;
    ASSERT((kbd_get_modifier_mask(&s) & 2) != 0, "ctrl one-shot → mask bit 1");
    s.alt_state = KBD_MODIFIER_LOCKED;
    ASSERT((kbd_get_modifier_mask(&s) & 4) != 0, "alt locked → mask bit 2");

    /* Reset */
    kbd_reset(&s);
    ASSERT(s.active_layer == KEYBOARD_LAYER_NORMAL, "reset: layer");
    ASSERT(s.ctrl_state == KBD_MODIFIER_OFF, "reset: ctrl");
    ASSERT(s.alt_state == KBD_MODIFIER_OFF, "reset: alt");
    ASSERT(s.meta_state == KBD_MODIFIER_OFF, "reset: meta");

    /* FN layer tests */
    kbd_reset(&s);
    ASSERT(s.fn_state == KBD_MODIFIER_OFF, "FN not active initially");

    kbd_toggle_fn(&s);
    ASSERT(s.fn_state == KBD_MODIFIER_ONESHOT, "FN one-shot active");
    kbd_toggle_fn(&s);
    ASSERT(s.fn_state == KBD_MODIFIER_LOCKED, "FN locked after second press");
    kbd_toggle_fn(&s);
    ASSERT(s.fn_state == KBD_MODIFIER_OFF, "FN off after third press");

    /* FN one-shot: sent key → auto-release */
    kbd_reset(&s);
    kbd_toggle_fn(&s);
    KeyDef non_fn_key = { .normal = XK_a, .label = "a", .flags = KEYFLAG_NORMAL };
    kbd_notify_key_sent(&s, &non_fn_key);
    ASSERT(s.fn_state == KBD_MODIFIER_OFF, "FN one-shot released after non-FN key");

    /* Effective layer check */
    kbd_reset(&s);
    kbd_toggle_fn(&s);
    KeyDef any_key = { .normal = XK_1, .label = "1", .flags = KEYFLAG_NORMAL };
    ASSERT(kbd_get_effective_layer(&s, &any_key) == KEYBOARD_LAYER_FN, "FN active → FN layer");

    printf("\n%d tests, %d failures\n", tests, failures);
    return failures > 0 ? 1 : 0;
}
