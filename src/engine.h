#ifndef XVKBD_ENGINE_H
#define XVKBD_ENGINE_H

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdbool.h>

typedef struct {
    Display *display;
    bool use_xtest;
    bool debug;
} EngineConfig;

int engine_init(EngineConfig *config);
void engine_send_key(KeySym keysym, bool pressed);
void engine_send_string(const char *str);
void engine_close(void);

#endif
