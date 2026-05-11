// SPDX-License-Identifier: MIT — see LICENSE file

#include "x11_cairo_bridge.h"
#include "cairo_renderer.h"
#include <cairo/cairo-xlib.h>
#include <stdio.h>
#include <stdlib.h>

// Internal data for X11+Cairo double buffering
typedef struct {
    X11Window *window;
    Pixmap pixmap;
    int width;
    int height;
} X11CairoData;

// Free function for X11CairoData
static void free_x11_cairo_data(void *ptr) {
    X11CairoData *data = (X11CairoData*)ptr;
    if (!data) return;
    
    if (data->pixmap && data->window) {
        Display *dpy = x11_window_get_display(data->window);
        if (dpy) {
            XFreePixmap(dpy, data->pixmap);
        }
    }
    free(data);
}

// Forward declaration of present function
static void x11_cairo_present(Renderer *renderer);

static void x11_cairo_present(Renderer *renderer) {
    if (!renderer) return;
    
    X11CairoData *data = (X11CairoData*)renderer_get_user_data(renderer);
    if (!data || !data->window) return;
    
    // Copy the pixmap to the window
    x11_window_copy_area(data->window, data->pixmap,
                         0, 0, data->width, data->height,
                         0, 0);
    
    // Flush X11 commands
    XFlush(x11_window_get_display(data->window));
}

void renderer_present_to_window(Renderer *renderer) {
    if (!renderer) return;
    x11_cairo_present(renderer);
}

Renderer* renderer_create_for_x11_window(X11Window *window, RendererConfig *config) {
    if (!window || !config) {
        fprintf(stderr, "renderer_create_for_x11_window: invalid parameters\n");
        return NULL;
    }
    
    // Get X11 resources
    Display *display = x11_window_get_display(window);
    Visual *visual = x11_window_get_visual(window);
    int depth = x11_window_get_depth(window);
    (void)depth; // Currently unused, but kept for future double buffering
    
    if (!display || !visual) {
        fprintf(stderr, "renderer_create_for_x11_window: failed to get X11 resources\n");
        return NULL;
    }
    
    // Create pixmap for double buffering
    Pixmap pixmap = x11_window_create_pixmap(window, config->width, config->height);
    if (!pixmap) {
        fprintf(stderr, "renderer_create_for_x11_window: failed to create pixmap\n");
        return NULL;
    }
    
    // Create Cairo Xlib surface targeting the pixmap (off-screen)
    cairo_surface_t *surface = cairo_xlib_surface_create(
        display, pixmap, visual, config->width, config->height);
    
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        fprintf(stderr, "renderer_create_for_x11_window: failed to create Cairo surface: %s\n",
                cairo_status_to_string(cairo_surface_status(surface)));
        XFreePixmap(display, pixmap);
        cairo_surface_destroy(surface);
        return NULL;
    }
    
    // Create Cairo renderer with the surface
    Renderer *renderer = cairo_renderer_create(config, surface);
    if (!renderer) {
        fprintf(stderr, "renderer_create_for_x11_window: failed to create Cairo renderer\n");
        XFreePixmap(display, pixmap);
        cairo_surface_destroy(surface);
        return NULL;
    }
    
    // Allocate X11CairoData to store window and pixmap reference
    X11CairoData *data = malloc(sizeof(X11CairoData));
    if (!data) {
        fprintf(stderr, "renderer_create_for_x11_window: out of memory\n");
        renderer_destroy(renderer); // will destroy surface and renderer, pixmap not attached yet
        XFreePixmap(display, pixmap);
        return NULL;
    }
    
    data->window = window;
    data->pixmap = pixmap;
    data->width = config->width;
    data->height = config->height;
    
    // Attach data to renderer with free function
    renderer_set_user_data_with_free(renderer, data, free_x11_cairo_data);
    
    return renderer;
}

void renderer_resize(Renderer *renderer, int width, int height) {
    if (!renderer) return;
    
    X11CairoData *data = (X11CairoData*)renderer_get_user_data(renderer);
    if (!data || !data->window) return;
    
    Display *display = x11_window_get_display(data->window);
    Visual *visual = x11_window_get_visual(data->window);
    
    // 1. Destroy old pixmap
    if (data->pixmap) {
        XFreePixmap(display, data->pixmap);
    }
    
    // 2. Create new pixmap
    data->pixmap = x11_window_create_pixmap(data->window, width, height);
    data->width = width;
    data->height = height;
    
    // 3. Create new Cairo surface
    cairo_surface_t *surface = cairo_xlib_surface_create(
        display, data->pixmap, visual, width, height);
    
    // 4. Update renderer
    cairo_renderer_update_surface(renderer, surface, width, height);
}