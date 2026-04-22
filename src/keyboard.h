#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "layout.h"
#include <stdbool.h>

// Forward declaration
typedef struct Keyboard Keyboard;

// Keyboard layer (shift state)
typedef enum {
    KEYBOARD_LAYER_NORMAL,
    KEYBOARD_LAYER_SHIFT,
    KEYBOARD_LAYER_ALTGR,
    NUM_KEYBOARD_LAYERS
} KeyboardLayer;

// Keyboard state
typedef struct {
    KeyboardLayer current_layer;
    bool caps_lock;
    bool num_lock;
    bool scroll_lock;
    int pressed_key_index;
    bool dirty; // Whether visual state needs update
} KeyboardState;

// Create a new keyboard instance
Keyboard* keyboard_create(Layout *layout);

// Set the keyboard layout
void keyboard_set_layout(Keyboard *kb, Layout *layout);

// Get the current layout
Layout* keyboard_get_layout(const Keyboard *kb);

// Press a key by index
void keyboard_press_key(Keyboard *kb, int key_index);

// Notify that a key was sent to the system (for one-shot logic)
void keyboard_notify_key_sent(Keyboard *kb, int key_index);

// Release a key by index  
void keyboard_release_key(Keyboard *kb, int key_index);

// Get the keysym for a key in current layer
KeySym keyboard_get_keysym(const Keyboard *kb, int key_index);

// Get the label for a key in current layer
const char* keyboard_get_key_label(const Keyboard *kb, int key_index);

// Get the effective layer for a specific key
int keyboard_get_effective_layer(const Keyboard *kb, int key_index);

// Toggle shift layer
void keyboard_toggle_shift(Keyboard *kb);

// Toggle caps lock
void keyboard_toggle_caps_lock(Keyboard *kb);

// Get current keyboard state
KeyboardState keyboard_get_state(const Keyboard *kb);

// Check if keyboard state is dirty (needs visual update)
bool keyboard_is_dirty(const Keyboard *kb);

// Mark keyboard as clean (visual update done)
void keyboard_mark_clean(Keyboard *kb);

// Destroy keyboard
void keyboard_destroy(Keyboard *kb);

#endif // KEYBOARD_H