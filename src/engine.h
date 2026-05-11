/*
 * 0-Board Virtual Keyboard
 * Copyright (c) 2026 Leonardo Vergara <leonardovergaramarin@gmail.com>
 * Licensed under the MIT License.
 */
#ifndef ENGINE_H
#define ENGINE_H

#include <X11/Xlib.h>
#include <stdbool.h>

// Forward declaration
typedef struct Engine Engine;

// Configuration for keyboard event engine
typedef struct {
    Display *display;       // NULL to open default display
    bool use_xtest;         // Use XTest extension for synthetic events
    int event_delay_us;     // Microseconds delay between press/release (0 = no delay)
} EngineConfig;

// Create a new engine instance
Engine* engine_create(EngineConfig *config);

// Send a key event (press or release)
int engine_send_key(Engine *engine, KeySym keysym, bool pressed);
int engine_send_key_ex(Engine *engine, KeySym keysym, bool pressed, int modifiers);

// Flush pending events to server
void engine_flush(Engine *engine);

// Destroy engine and release resources
void engine_destroy(Engine *engine);

#endif // ENGINE_H