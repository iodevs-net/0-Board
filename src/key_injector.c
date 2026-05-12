// SPDX-License-Identifier: MIT — see LICENSE file
//
// key_injector.c — Inyección de teclas con delay entre press/release.
// El flush X11 lo maneja inject_key_sequence() en engine.c.
// key_injector_send solo añade el delay — no duplica flushes.

#include "key_injector.h"
#include "layout.h"
#include <unistd.h>
#include <X11/keysym.h>

bool key_injector_is_modifier(KeyDef *key) {
    if (!key) return false;
    return (key->flags & (KEYFLAG_SHIFT | KEYFLAG_CTRL | KEYFLAG_ALT
                         | KEYFLAG_META | KEYFLAG_FN)) ||
           (key->normal == XK_Caps_Lock);
}

void key_injector_send(Engine *engine, Display *display,
                       KeySym sym, int modifiers, int key_event_delay_us) {
    if (!engine || !display) return;

    engine_send_key_ex(engine, sym, true, modifiers);

    if (key_event_delay_us > 0)
        usleep(key_event_delay_us);

    engine_send_key_ex(engine, sym, false, modifiers);
    /* engine_send_key_ex ya hace XFlush por cada inyección;
     * no duplicamos flush aquí. */
}
