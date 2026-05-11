// SPDX-License-Identifier: MIT — see LICENSE file

#ifndef RENDERER_H
#define RENDERER_H

#include <stdbool.h>

// Forward declarations
typedef struct Renderer Renderer;

// Rectangle for positioning
typedef struct Rectangle {
    double x;
    double y;
    double width;
    double height;
} Rectangle;

// Text alignment options
typedef enum {
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT
} TextAlignment;

// Vertical alignment options  
typedef enum {
    VALIGN_TOP,
    VALIGN_CENTER,
    VALIGN_BOTTOM
} VerticalAlignment;

// Color with alpha
typedef struct {
    double red;
    double green;
    double blue;
    double alpha;
} Color;

// Font specification
typedef struct {
    const char *family;
    double size;
    bool bold;
    bool italic;
} FontSpec;

// Renderer configuration
typedef struct {
    int width;
    int height;
    double default_opacity;
    Color clear_color;
} RendererConfig;

// Create a renderer (implementation-specific)
Renderer* renderer_create(RendererConfig *config);

// Begin a new frame
void renderer_begin_frame(Renderer *renderer);

// End frame and present
void renderer_end_frame(Renderer *renderer);

// Clear the render target
void renderer_clear(Renderer *renderer, Color color);

// Draw a filled rectangle
void renderer_draw_rectangle(Renderer *renderer, Rectangle rect, Color color, double corner_radius);

// Draw rectangle outline
void renderer_draw_rectangle_outline(Renderer *renderer, Rectangle rect, Color color, double line_width, double corner_radius);

// Draw text
void renderer_draw_text(Renderer *renderer, const char *text, Rectangle bounds, 
                        FontSpec font, Color color, 
                        TextAlignment halign, VerticalAlignment valign);

// Set clipping rectangle
void renderer_set_clip(Renderer *renderer, Rectangle rect);

// Clear clipping rectangle
void renderer_clear_clip(Renderer *renderer);

// Get text dimensions
Rectangle renderer_measure_text(Renderer *renderer, const char *text, FontSpec font);

// Resize render target
void renderer_resize(Renderer *renderer, int width, int height);

// Surface caching (KISS)
typedef struct RendererSurface RendererSurface;
RendererSurface* renderer_create_surface(Renderer *renderer, int width, int height);
void renderer_draw_to_surface(Renderer *renderer, RendererSurface *surface, void (*draw_func)(Renderer*, void*), void *user_data);
void renderer_draw_surface(Renderer *renderer, RendererSurface *surface, double x, double y, double opacity);
void renderer_destroy_surface(RendererSurface *surface);

// Destroy renderer
void renderer_destroy(Renderer *renderer);

// User data storage (implementation-specific)
void renderer_set_user_data(Renderer *renderer, void *user_data);
void renderer_set_user_data_with_free(Renderer *renderer, void *user_data, void (*free_func)(void*));
void* renderer_get_user_data(Renderer *renderer);

#endif // RENDERER_H