#include <stdio.h>
#include <string.h>
#include <X11/keysym.h>
#include "keyboard_state.h"

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

    KbdState s;
    kbd_state_reset(&s);

    // Initial state
    ASSERT(s.active_layer == LAYER_NORMAL, "initial layer is NORMAL");
    ASSERT(!s.shift_locked, "initial shift not locked");
    ASSERT(s.pressed_key_index == -1, "initial no pressed key");
    ASSERT(s.dirty == true, "reset marks dirty");

    // Shift: normal → one-shot → locked → off
    kbd_state_toggle_shift(&s);
    ASSERT(s.active_layer == LAYER_SHIFT, "toggle shift: one-shot");
    ASSERT(!s.shift_locked, "toggle shift: not locked yet");

    kbd_state_toggle_shift(&s);
    ASSERT(s.shift_locked == true, "toggle shift: locked");
    ASSERT(s.active_layer == LAYER_SHIFT, "toggle shift: still shift layer");

    kbd_state_toggle_shift(&s);
    ASSERT(s.active_layer == LAYER_NORMAL, "toggle shift: off");
    ASSERT(!s.shift_locked, "toggle shift: unlocked");

    // One-shot: key sent → auto-release
    kbd_state_reset(&s);
    kbd_state_toggle_shift(&s); // one-shot
    KeyDef non_shift_key = { .normal = XK_a, .label = "a", .flags = KEYFLAG_NORMAL };
    kbd_state_notify_key_sent(&s, &non_shift_key);
    ASSERT(s.active_layer == LAYER_NORMAL, "one-shot released after non-modifier key");

    // Caps Lock
    kbd_state_reset(&s);
    kbd_state_toggle_caps(&s);
    ASSERT(s.caps_lock == true, "caps on");
    KeyDef letter = { .normal = XK_a, .label = "a", .flags = KEYFLAG_NORMAL };
    ASSERT(kbd_state_get_effective_layer(&s, &letter) == LAYER_SHIFT, "caps → shift for letters");
    KeyDef number = { .normal = XK_1, .label = "1", .flags = KEYFLAG_NORMAL };
    ASSERT(kbd_state_get_effective_layer(&s, &number) == LAYER_NORMAL, "caps → normal for numbers");

    // Modifier mask
    kbd_state_reset(&s);
    ASSERT(kbd_state_get_modifier_mask(&s) == 0, "no modifiers = mask 0");
    s.ctrl_state = MODIFIER_ONESHOT;
    ASSERT((kbd_state_get_modifier_mask(&s) & 2) != 0, "ctrl one-shot → mask bit 1");
    s.alt_state = MODIFIER_LOCKED;
    ASSERT((kbd_state_get_modifier_mask(&s) & 4) != 0, "alt locked → mask bit 2");

    // Reset
    kbd_state_reset(&s);
    ASSERT(s.active_layer == LAYER_NORMAL, "reset: layer");
    ASSERT(s.ctrl_state == MODIFIER_OFF, "reset: ctrl");
    ASSERT(s.alt_state == MODIFIER_OFF, "reset: alt");
    ASSERT(s.meta_state == MODIFIER_OFF, "reset: meta");

    printf("\n%d tests, %d failures\n", tests, failures);
    return failures > 0 ? 1 : 0;
}
