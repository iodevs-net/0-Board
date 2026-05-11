// SPDX-License-Identifier: MIT — see LICENSE file

#ifndef X11_CAIRO_BRIDGE_H
#define X11_CAIRO_BRIDGE_H

#include "renderer.h"
#include "x11_window.h"

// Create a Cairo renderer for an X11 window
// This creates a cairo_xlib_surface_t targeting the window's pixmap
Renderer* renderer_create_for_x11_window(X11Window *window, RendererConfig *config);

// Present the rendered content to the window (double buffering)
void renderer_present_to_window(Renderer *renderer);

// Resize the X11 surface and pixmap
void renderer_resize(Renderer *renderer, int width, int height);

#endif // X11_CAIRO_BRIDGE_H