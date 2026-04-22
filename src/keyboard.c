#include "keyboard.h"
#include "keyboard_state.h"
#include <X11/keysym.h>
#include <stdlib.h>
#include <string.h>

struct Keyboard {
    Layout *layout;
    KbdState state;
    bool owns_layout;
};

Keyboard* keyboard_create(Layout *layout) {
    Keyboard *kb = calloc(1, sizeof(Keyboard));
    if (!kb) return NULL;
    
    kb->layout = layout;
    kbd_state_reset(&kb->state);
    return kb;
}

void keyboard_press_key(Keyboard *kb, int key_index) {
    if (!kb || !kb->layout || key_index < 0 || key_index >= kb->layout->num_keys) return;
    
    kb->state.pressed_key_index = key_index;
    
    KeyDef *key = &kb->layout->keys[key_index];
    if (key->flags & KEYFLAG_SHIFT) {
        kbd_state_toggle_shift(&kb->state);
    } else if (key->normal == XK_Caps_Lock) {
        kbd_state_toggle_caps(&kb->state);
    }
    
    kb->state.dirty = true;
}

void keyboard_release_key(Keyboard *kb, int key_index) {
    if (!kb) return;
    (void)key_index;
    kb->state.pressed_key_index = -1;
    kb->state.dirty = true;
}

void keyboard_notify_key_sent(Keyboard *kb, int key_index) {
    if (!kb || !kb->layout || key_index < 0 || key_index >= kb->layout->num_keys) return;
    
    KeyDef *key = &kb->layout->keys[key_index];
    kbd_state_notify_key_sent(&kb->state, key);
}

KeySym keyboard_get_keysym(const Keyboard *kb, int key_index) {
    if (!kb || !kb->layout || key_index < 0 || key_index >= kb->layout->num_keys) return 0;
    
    KeyDef *key = &kb->layout->keys[key_index];
    Layer effective_layer = kbd_state_get_effective_layer(&kb->state, key);
    
    if (effective_layer == LAYER_SHIFT && key->shifted != 0) return key->shifted;
    if (effective_layer == LAYER_ALTGR && key->altgr != 0) return key->altgr;
    
    return key->normal;
}

int keyboard_get_effective_layer(const Keyboard *kb, int key_index) {
    if (!kb || !kb->layout || key_index < 0 || key_index >= kb->layout->num_keys) return 0;
    KeyDef *key = &kb->layout->keys[key_index];
    return (int)kbd_state_get_effective_layer(&kb->state, key);
}

const char* keyboard_get_key_label(const Keyboard *kb, int key_index) {
    if (!kb || !kb->layout || key_index < 0 || key_index >= kb->layout->num_keys) return NULL;
    
    KeyDef *key = &kb->layout->keys[key_index];
    Layer effective_layer = kbd_state_get_effective_layer(&kb->state, key);
    
    if (effective_layer == LAYER_SHIFT && key->shifted_label) return key->shifted_label;
    
    return key->label;
}

bool keyboard_is_dirty(const Keyboard *kb) { return kb ? kb->state.dirty : false; }
void keyboard_mark_clean(Keyboard *kb) { if (kb) kb->state.dirty = false; }
Layout* keyboard_get_layout(const Keyboard *kb) { return kb ? kb->layout : NULL; }

void keyboard_destroy(Keyboard *kb) {
    if (!kb) return;
    if (kb->owns_layout && kb->layout) { /* free logic */ }
    free(kb);
}

// Stubs for remaining functions in keyboard.h if needed
void keyboard_set_layout(Keyboard *kb, Layout *layout) { if (kb) kb->layout = layout; }
void keyboard_toggle_shift(Keyboard *kb) { if (kb) kbd_state_toggle_shift(&kb->state); }
void keyboard_toggle_caps_lock(Keyboard *kb) { if (kb) kbd_state_toggle_caps(&kb->state); }
KeyboardState keyboard_get_state(const Keyboard *kb) { 
    KeyboardState s = {0}; 
    if (kb) {
        s.current_layer = (KeyboardLayer)kb->state.active_layer;
        s.caps_lock = kb->state.caps_lock;
        s.pressed_key_index = kb->state.pressed_key_index;
        s.dirty = kb->state.dirty;
    }
    return s; 
}