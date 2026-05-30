// SPDX-License-Identifier: MIT — see LICENSE file

#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XTest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

static unsigned long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

int main() {
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Unable to open display\n");
        return 1;
    }

    // Check XInput2 extension
    int ev, err;
    if (!XQueryExtension(display, "XInputExtension", &ev, &err, &ev)) {
        fprintf(stderr, "XInput extension not available\n");
        XCloseDisplay(display);
        return 1;
    }

    // Get root window
    Window root = DefaultRootWindow(display);

    // Select raw pointer events on root window
    unsigned char mask_bytes[XIMaskLen(XI_LASTEVENT)] = {0};
    XISetMask(mask_bytes, XI_RawButtonPress);
    XISetMask(mask_bytes, XI_RawButtonRelease);

    XIEventMask mask;
    mask.deviceid = XIAllMasterDevices; // Master pointer/keyboard
    mask.mask_len = sizeof(mask_bytes);
    mask.mask = mask_bytes;

    if (XISelectEvents(display, root, &mask, 1) != Success) {
        fprintf(stderr, "XISelectEvents failed\n");
        XCloseDisplay(display);
        return 1;
    }
    XFlush(display);

    unsigned long press_time = 0;
    int press_x = 0, press_y = 0;
    bool pressed = false;

    printf("0-board-touch-handler started.\n");

    while (1) {
        XEvent ev;
        XNextEvent(display, &ev);

        if (ev.type == GenericEvent) {
            if (XGetEventData(display, &ev.xcookie)) {
                if (ev.xcookie.evtype == XI_RawButtonPress) {
                    XIRawEvent *re = (XIRawEvent*)ev.xcookie.data;
                    if (re->detail == 1) { // Left button press
                        pressed = true;
                        press_time = now_ms();
                        
                        Window root_ret, child_ret;
                        int root_x, root_y, win_x, win_y;
                        unsigned int mask_ret;
                        XQueryPointer(display, root, &root_ret, &child_ret,
                                      &root_x, &root_y, &win_x, &win_y, &mask_ret);
                        press_x = root_x;
                        press_y = root_y;
                    }
                } else if (ev.xcookie.evtype == XI_RawButtonRelease) {
                    XIRawEvent *re = (XIRawEvent*)ev.xcookie.data;
                    if (re->detail == 1 && pressed) {
                        pressed = false;
                        unsigned long elapsed = now_ms() - press_time;
                        
                        Window root_ret, child_ret;
                        int root_x, root_y, win_x, win_y;
                        unsigned int mask_ret;
                        XQueryPointer(display, root, &root_ret, &child_ret,
                                      &root_x, &root_y, &win_x, &win_y, &mask_ret);
                        
                        int dx = root_x - press_x;
                        int dy = root_y - press_y;
                        int dist_sq = dx*dx + dy*dy;
                        
                        // If held for 600ms and pointer didn't drift more than 15px
                        if (elapsed >= 600 && dist_sq < 225) {
                            // Inject Right Click (button 3)
                            XTestFakeButtonEvent(display, 3, True, 0);
                            XFlush(display);
                            usleep(20000);
                            XTestFakeButtonEvent(display, 3, False, 0);
                            XFlush(display);
                        }
                    }
                }
                XFreeEventData(display, &ev.xcookie);
            }
        }
    }

    XCloseDisplay(display);
    return 0;
}
