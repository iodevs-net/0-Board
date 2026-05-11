// SPDX-License-Identifier: MIT — see LICENSE file

#ifndef KEY_INJECTOR_H
#define KEY_INJECTOR_H

#include <X11/Xlib.h>
#include <stdbool.h>
#include "engine.h"
#include "layout.h"

// Check if a key is a modifier (should only toggle state, not send characters)
bool key_injector_is_modifier(KeyDef *key);

// Send a key press+release with proper modifiers and delay
void key_injector_send(Engine *engine, Display *display,
                       KeySym sym, int modifiers, int key_event_delay_us);

#endif // KEY_INJECTOR_H
