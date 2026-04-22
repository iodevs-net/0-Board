#ifndef X11_WINDOW_H
#define X11_WINDOW_H

#include <X11/Xlib.h>
#include <stdbool.h>

// Forward declaration
typedef struct X11Window X11Window;

// Window configuration
typedef struct {
    int x, y;                 // Position (-1 for center)
    int width, height;        // Size (-1 for auto-detect)
    int initial_size_index;   // 0=small, 1=medium, 2=large (for auto-size)
    const char *title;        // Window title
    bool borderless;          // Whether to show window decorations
    bool override_redirect;   // Bypass window manager
    bool skip_taskbar;        // Don't show in taskbar/dock
    double opacity;           // 0.0 to 1.0
} WindowConfig;

// Event types
typedef enum {
    WINDOW_EVENT_EXPOSE,
    WINDOW_EVENT_BUTTON_PRESS,
    WINDOW_EVENT_BUTTON_RELEASE,
    WINDOW_EVENT_MOTION,
    WINDOW_EVENT_CLOSE,
    WINDOW_EVENT_RESIZE,
    WINDOW_EVENT_KEY_PRESS,
    WINDOW_EVENT_KEY_RELEASE
} WindowEventType;

// Mouse button
typedef enum {
    MOUSE_BUTTON_LEFT = 1,
    MOUSE_BUTTON_MIDDLE = 2,
    MOUSE_BUTTON_RIGHT = 3
} MouseButton;

// Event data
typedef struct {
    WindowEventType type;
    int x, y;                    // Mouse position (window-relative)
    int root_x, root_y;         // Mouse position (screen-relative, for dragging)
    MouseButton button;          // For button events
    unsigned int state;          // Modifier keys state
    unsigned int keycode;        // For key events
    int width, height;           // For resize events
} WindowEvent;

// Event callback type
typedef void (*WindowEventCallback)(X11Window *window, WindowEvent *event, void *user_data);

// Create a new X11 window
X11Window* x11_window_create(WindowConfig *config);

// Set event callback
void x11_window_set_event_callback(X11Window *window, WindowEventCallback callback, void *user_data);

// Get X11 display
Display* x11_window_get_display(X11Window *window);

// Get X11 window ID
Window x11_window_get_id(X11Window *window);

// Get visual information
Visual* x11_window_get_visual(X11Window *window);
int x11_window_get_depth(X11Window *window);

// Get drawable (pixmap or window)
Pixmap x11_window_create_pixmap(X11Window *window, int width, int height);

// Copy pixmap to window
void x11_window_copy_area(X11Window *window, Pixmap src, 
                          int src_x, int src_y, int src_width, int src_height,
                          int dest_x, int dest_y);

// Move window
void x11_window_move(X11Window *window, int x, int y);

// Get window position
void x11_window_get_position(X11Window *window, int *x, int *y);

// Resize window
void x11_window_resize(X11Window *window, int width, int height);

// Move and resize window atomically
void x11_window_move_resize(X11Window *window, int x, int y, int width, int height);

// Get current window size
void x11_window_get_size(X11Window *window, int *width, int *height);

// Set window title
void x11_window_set_title(X11Window *window, const char *title);

// Set window opacity (requires composite manager)
void x11_window_set_opacity(X11Window *window, double opacity);

// Set Always on Top
void x11_window_set_always_on_top(X11Window *window, bool on_top);

// Show window
void x11_window_show(X11Window *window);

// Hide window
void x11_window_hide(X11Window *window);

// Process events (non-blocking)
bool x11_window_process_events(X11Window *window);

// Process events (blocking with timeout)
bool x11_window_wait_event(X11Window *window, int timeout_ms);

// Close window
void x11_window_close(X11Window *window);

// Destroy window
void x11_window_destroy(X11Window *window);

#endif // X11_WINDOW_H