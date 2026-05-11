// SPDX-License-Identifier: MIT — see LICENSE file

#include "cairo_renderer.h"
#include <cairo/cairo.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define CAIRO_PI 3.14159265358979323846

struct Renderer {
    cairo_t *cr;
    cairo_surface_t *surface;
    RendererConfig config;
    bool owns_surface;
    void *user_data;
    void (*user_data_free)(void*);
};

// Helper to convert our Color to Cairo's pattern
static void set_cairo_color(cairo_t *cr, Color color) {
    cairo_set_source_rgba(cr, color.red, color.green, color.blue, color.alpha);
}

// Helper to draw rounded rectangle path
static void cairo_rounded_rectangle(cairo_t *cr, double x, double y, 
                                    double width, double height, double radius) {
    if (radius <= 0) {
        cairo_rectangle(cr, x, y, width, height);
        return;
    }
    
    // Clamp radius to half the smallest dimension
    double max_radius = fmin(width, height) / 2.0;
    if (radius > max_radius) {
        radius = max_radius;
    }
    
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + width - radius, y + radius, radius, -CAIRO_PI/2, 0);
    cairo_arc(cr, x + width - radius, y + height - radius, radius, 0, CAIRO_PI/2);
    cairo_arc(cr, x + radius, y + height - radius, radius, CAIRO_PI/2, CAIRO_PI);
    cairo_arc(cr, x + radius, y + radius, radius, CAIRO_PI, 3*CAIRO_PI/2);
    cairo_close_path(cr);
}

Renderer* cairo_renderer_create(RendererConfig *config, void *native_surface) {
    if (!config) {
        return NULL;
    }
    
    Renderer *renderer = malloc(sizeof(Renderer));
    if (!renderer) {
        return NULL;
    }
    
    renderer->config = *config;
    renderer->owns_surface = (native_surface == NULL);
    renderer->user_data = NULL;
    renderer->user_data_free = NULL;
    
    if (native_surface) {
        renderer->surface = (cairo_surface_t*)native_surface;
    } else {
        // Create an image surface as fallback
        renderer->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 
                                                      config->width, config->height);
    }
    
    if (!renderer->surface || cairo_surface_status(renderer->surface) != CAIRO_STATUS_SUCCESS) {
        free(renderer);
        return NULL;
    }
    
    renderer->cr = cairo_create(renderer->surface);
    if (!renderer->cr || cairo_status(renderer->cr) != CAIRO_STATUS_SUCCESS) {
        if (renderer->owns_surface) {
            cairo_surface_destroy(renderer->surface);
        }
        free(renderer);
        return NULL;
    }
    
    return renderer;
}

void* cairo_renderer_get_context(Renderer *renderer) {
    return renderer ? renderer->cr : NULL;
}

void* cairo_renderer_get_surface(Renderer *renderer) {
    return renderer ? renderer->surface : NULL;
}

void renderer_begin_frame(Renderer *renderer) {
    // Cairo doesn't need explicit begin/end frame
    // But we could save state here if needed
    if (renderer && renderer->cr) {
        cairo_save(renderer->cr);
    }
}

void renderer_end_frame(Renderer *renderer) {
    if (renderer && renderer->cr) {
        cairo_restore(renderer->cr);
    }
}

void renderer_clear(Renderer *renderer, Color color) {
    if (!renderer || !renderer->cr) return;
    
    // Save current operator
    cairo_operator_t old_op = cairo_get_operator(renderer->cr);
    
    // Clear with source operator (replaces everything)
    cairo_set_operator(renderer->cr, CAIRO_OPERATOR_SOURCE);
    set_cairo_color(renderer->cr, color);
    cairo_paint(renderer->cr);
    
    // Restore operator
    cairo_set_operator(renderer->cr, old_op);
}

void renderer_draw_rectangle(Renderer *renderer, Rectangle rect, Color color, double corner_radius) {
    if (!renderer || !renderer->cr) return;
    
    cairo_rounded_rectangle(renderer->cr, rect.x, rect.y, rect.width, rect.height, corner_radius);
    set_cairo_color(renderer->cr, color);
    cairo_fill(renderer->cr);
}

void renderer_draw_rectangle_outline(Renderer *renderer, Rectangle rect, Color color, 
                                     double line_width, double corner_radius) {
    if (!renderer || !renderer->cr) return;
    
    cairo_rounded_rectangle(renderer->cr, rect.x, rect.y, rect.width, rect.height, corner_radius);
    set_cairo_color(renderer->cr, color);
    cairo_set_line_width(renderer->cr, line_width);
    cairo_stroke(renderer->cr);
}

void renderer_draw_text(Renderer *renderer, const char *text, Rectangle bounds, 
                        FontSpec font, Color color, 
                        TextAlignment halign, VerticalAlignment valign) {
    if (!renderer || !renderer->cr || !text) return;
    
    // Set font
    cairo_font_slant_t slant = font.italic ? CAIRO_FONT_SLANT_ITALIC : CAIRO_FONT_SLANT_NORMAL;
    cairo_font_weight_t weight = font.bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL;
    
    cairo_select_font_face(renderer->cr, font.family ? font.family : "sans-serif", 
                          slant, weight);
    cairo_set_font_size(renderer->cr, font.size);
    
    // Get text extents
    cairo_text_extents_t extents;
    cairo_text_extents(renderer->cr, text, &extents);
    
    // Calculate position based on alignment
    double x, y;
    
    switch (halign) {
        case ALIGN_LEFT:
            x = bounds.x;
            break;
        case ALIGN_CENTER:
            x = bounds.x + bounds.width/2 - extents.width/2 - extents.x_bearing;
            break;
        case ALIGN_RIGHT:
            x = bounds.x + bounds.width - extents.width - extents.x_bearing;
            break;
        default:
            x = bounds.x;
    }
    
    switch (valign) {
        case VALIGN_TOP:
            y = bounds.y - extents.y_bearing;
            break;
        case VALIGN_CENTER:
            y = bounds.y + bounds.height/2 - extents.height/2 - extents.y_bearing;
            break;
        case VALIGN_BOTTOM:
            y = bounds.y + bounds.height - extents.height - extents.y_bearing;
            break;
        default:
            y = bounds.y;
    }
    
    // Draw text
    set_cairo_color(renderer->cr, color);
    cairo_move_to(renderer->cr, x, y);
    cairo_show_text(renderer->cr, text);
}

void renderer_set_clip(Renderer *renderer, Rectangle rect) {
    if (!renderer || !renderer->cr) return;
    
    cairo_rectangle(renderer->cr, rect.x, rect.y, rect.width, rect.height);
    cairo_clip(renderer->cr);
}

void renderer_clear_clip(Renderer *renderer) {
    if (!renderer || !renderer->cr) return;
    
    cairo_reset_clip(renderer->cr);
}

Rectangle renderer_measure_text(Renderer *renderer, const char *text, FontSpec font) {
    Rectangle result = {0};
    
    if (!renderer || !renderer->cr || !text) return result;
    
    // Set font temporarily
    cairo_font_slant_t slant = font.italic ? CAIRO_FONT_SLANT_ITALIC : CAIRO_FONT_SLANT_NORMAL;
    cairo_font_weight_t weight = font.bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL;
    
    cairo_select_font_face(renderer->cr, font.family ? font.family : "sans-serif", 
                          slant, weight);
    cairo_set_font_size(renderer->cr, font.size);
    
    // Get extents
    cairo_text_extents_t extents;
    cairo_text_extents(renderer->cr, text, &extents);
    
    result.width = extents.width;
    result.height = extents.height;
    
    return result;
}

void cairo_renderer_update_surface(Renderer *renderer, void *new_surface, int width, int height) {
    if (!renderer || !new_surface) return;
    
    // Destroy old context and surface if we own it
    if (renderer->cr) {
        cairo_destroy(renderer->cr);
    }
    if (renderer->owns_surface && renderer->surface) {
        cairo_surface_destroy(renderer->surface);
    }
    
    renderer->surface = (cairo_surface_t*)new_surface;
    renderer->cr = cairo_create(renderer->surface);
    renderer->config.width = width;
    renderer->config.height = height;
    renderer->owns_surface = true; // We take ownership in this bridge case
}


void renderer_destroy(Renderer *renderer) {
    if (!renderer) return;
    if (renderer->user_data && renderer->user_data_free) {
        renderer->user_data_free(renderer->user_data);
    }
    cairo_destroy(renderer->cr);
    if (renderer->owns_surface) {
        cairo_surface_destroy(renderer->surface);
    }
    free(renderer);
}

struct RendererSurface {
    cairo_surface_t *surface;
};

RendererSurface* renderer_create_surface(Renderer *renderer, int width, int height) {
    if (!renderer) return NULL;
    RendererSurface *rs = malloc(sizeof(RendererSurface));
    rs->surface = cairo_surface_create_similar(renderer->surface, CAIRO_CONTENT_COLOR_ALPHA, width, height);
    return rs;
}

void renderer_draw_to_surface(Renderer *renderer, RendererSurface *surface, void (*draw_func)(Renderer*, void*), void *user_data) {
    if (!renderer || !surface) return;
    cairo_surface_t *old_surface = renderer->surface;
    cairo_t *old_cr = renderer->cr;
    
    renderer->surface = surface->surface;
    renderer->cr = cairo_create(renderer->surface);
    
    draw_func(renderer, user_data);
    
    cairo_destroy(renderer->cr);
    renderer->surface = old_surface;
    renderer->cr = old_cr;
}

void renderer_draw_surface(Renderer *renderer, RendererSurface *surface, double x, double y, double opacity) {
    if (!renderer || !surface) return;
    cairo_set_source_surface(renderer->cr, surface->surface, x, y);
    if (opacity >= 1.0) cairo_paint(renderer->cr);
    else cairo_paint_with_alpha(renderer->cr, opacity);
}

void renderer_destroy_surface(RendererSurface *surface) {
    if (!surface) return;
    cairo_surface_destroy(surface->surface);
    free(surface);
}

void renderer_set_user_data(Renderer *renderer, void *user_data) {
    if (!renderer) return;
    renderer->user_data = user_data;
    renderer->user_data_free = NULL;
}

void renderer_set_user_data_with_free(Renderer *renderer, void *user_data, void (*free_func)(void*)) {
    if (!renderer) return;
    renderer->user_data = user_data;
    renderer->user_data_free = free_func;
}

void* renderer_get_user_data(Renderer *renderer) {
    return renderer ? renderer->user_data : NULL;
}
