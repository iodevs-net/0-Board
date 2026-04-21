#include "engine.h"
#include <X11/extensions/XTest.h>
#include <stdio.h>
#include <stdlib.h>

static Display *target_dpy = NULL;
static bool use_xtest = true;
static bool debug = false;

int engine_init(EngineConfig *config) {
    target_dpy = config->display;
    use_xtest = config->use_xtest;
    debug = config->debug;

    if (!target_dpy) {
        target_dpy = XOpenDisplay(NULL);
        if (!target_dpy) {
            fprintf(stderr, "Error: No se pudo abrir el display de X11.\n");
            return -1;
        }
    }
    return 0;
}

void engine_send_key(KeySym keysym, bool pressed) {
    if (!target_dpy) return;

    KeyCode keycode = XKeysymToKeycode(target_dpy, keysym);
    if (keycode == 0) {
        if (debug) fprintf(stderr, "Debug: KeySym %lu no tiene KeyCode asociado.\n", (unsigned long)keysym);
        return;
    }

    if (use_xtest) {
        XTestFakeKeyEvent(target_dpy, keycode, pressed, 0);
    } else {
        // Fallback a XSendEvent (más complejo, omitido por ahora por KISS)
        if (debug) fprintf(stderr, "Debug: XSendEvent no implementado aún.\n");
    }
    XFlush(target_dpy);
}

void engine_send_string(const char *str) {
    // Implementación simple para pruebas
    while (*str) {
        KeySym keysym = XStringToKeysym(str); // Muy simplificado
        if (keysym != NoSymbol) {
            engine_send_key(keysym, true);
            engine_send_key(keysym, false);
        }
        str++;
    }
}

void engine_close(void) {
    if (target_dpy) {
        // No cerramos si fue pasado externamente, pero aquí asumimos control por ahora
        // XCloseDisplay(target_dpy);
    }
}
