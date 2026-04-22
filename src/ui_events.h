#ifndef UI_EVENTS_H
#define UI_EVENTS_H

#include "x11_window.h"

typedef struct UI UI;

void ui_handle_button_press(UI *ui, int wx, int wy, int rx, int ry, int button);
void ui_handle_button_release(UI *ui, int x, int y, int button);
void ui_event_callback(X11Window *window, WindowEvent *event, void *user_data);

#endif // UI_EVENTS_H
