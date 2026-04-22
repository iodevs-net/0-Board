#ifndef UI_INTERNAL_H
#define UI_INTERNAL_H

#include "ui.h"
#include "renderer.h"
#include "x11_window.h"
#include "keyboard.h"
#include "font_manager.h"
#include "engine.h"

// Internal UI state shared across modules
struct UI {
    // Dependencies
    Keyboard *keyboard;
    Renderer *renderer;
    X11Window *window;
    FontManager *font_manager;
    Engine *engine;
    
    // Configuration
    UIConfig config;
    
    // State
    double opacity;
    int color_scheme_index;
    int size_index;
    bool menu_visible;
    bool docked_top;
    bool dirty;
    bool should_close;
    
    // Layout cache
    Rectangle *key_bounds;
    KeyVisualMetadata *key_metadata;
    int key_count;
    int current_width;
    int current_height;
    
    // Rendering cache
    RendererSurface *bg_cache;
    bool bg_dirty;
    int last_rendered_layer;
    bool last_rendered_caps;
    
    // Interaction state
    bool dragging;
    int drag_offset_x, drag_offset_y;
};

// Internal helpers
void ui_calculate_layout(UI *ui);

#endif // UI_INTERNAL_H
