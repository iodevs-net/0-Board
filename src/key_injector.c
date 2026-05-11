// SPDX-License-Identifier: MIT — see LICENSE file

#include "key_injector.h"
#include "layout.h"
#include <unistd.h>
#include <X11/keysym.h>

bool key_injector_is_modifier(KeyDef *key) {
    if (!key) return false;
    return (key->flags & (KEYFLAG_SHIFT | KEYFLAG_CTRL | KEYFLAG_ALT | KEYFLAG_META)) ||
           (key->normal == XK_Caps_Lock);
}

void key_injector_send(Engine *engine, Display *display,
                       KeySym sym, int modifiers, int key_event_delay_us) {
    if (!engine || !display) return;

    engine_send_key_ex(engine, sym, true, modifiers);
    XFlush(display);

    if (key_event_delay_us > 0) {
        usleep(key_event_delay_us);
    }

    engine_send_key_ex(engine, sym, false, modifiers);
    engine_flush(engine);
}
