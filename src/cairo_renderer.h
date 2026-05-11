// SPDX-License-Identifier: MIT — see LICENSE file

#ifndef CAIRO_RENDERER_H
#define CAIRO_RENDERER_H

#include "renderer.h"

// Cairo-specific renderer implementation
Renderer* cairo_renderer_create(RendererConfig *config, void *native_surface);

// Get the Cairo context (for advanced operations)
void* cairo_renderer_get_context(Renderer *renderer);

// Get the Cairo surface (for advanced operations)
void* cairo_renderer_get_surface(Renderer *renderer);

// Update surface (for resizing)
void cairo_renderer_update_surface(Renderer *renderer, void *new_surface, int width, int height);

#endif // CAIRO_RENDERER_H