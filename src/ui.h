#ifndef UI_H
#define UI_H

#include <stdbool.h>
#include <signal.h>

// Forward declarations
typedef struct UI UI;
typedef struct Keyboard Keyboard;
typedef struct Renderer Renderer;
typedef struct X11Window X11Window;
typedef struct FontManager FontManager;
typedef struct Engine Engine;

// Visual metadata for pre-calculated rendering
typedef struct {
    double font_size;
    bool bold;
    bool is_modifier;
    bool is_special;
    bool is_symbol;
} KeyVisualMetadata;

// UI configuration
typedef struct {
    const char *title;
    double initial_opacity;
    bool show_menu_bar;
    int initial_size; // 0=small, 1=medium, 2=large
} UIConfig;

// Create UI instance
UI* ui_create(UIConfig *config, Keyboard *keyboard, 
              Renderer *renderer, X11Window *window,
              FontManager *font_manager);

// Set the input engine
void ui_set_engine(UI *ui, Engine *engine);

// Run the UI main loop (blocks until close)
void ui_run(UI *ui);

// Run with external shutdown flag (for signal handling)
void ui_run_with_shutdown(UI *ui, volatile sig_atomic_t *shutdown_flag);

// Get current opacity
double ui_get_opacity(const UI *ui);

// Set opacity (0.0 to 1.0)
void ui_set_opacity(UI *ui, double opacity);

// Get current color scheme index
int ui_get_color_scheme(const UI *ui);

// Set color scheme by index
void ui_set_color_scheme(UI *ui, int scheme_index);

// Get current font family
const char* ui_get_font_family(const UI *ui);

// Set font family
bool ui_set_font_family(UI *ui, const char *family);

// Get current size index (0=small, 1=medium, 2=large)
int ui_get_size_index(const UI *ui);

// Set size by index
void ui_set_size_index(UI *ui, int size_index);

// Show menu bar
void ui_show_menu(UI *ui);

// Hide menu bar  
void ui_hide_menu(UI *ui);

// Toggle dock position (top/bottom center)
void ui_toggle_dock_position(UI *ui);

// Check if menu is visible
bool ui_is_menu_visible(const UI *ui);

// Mark UI as needing redraw
void ui_mark_dirty(UI *ui);

// Check if UI needs redraw
bool ui_is_dirty(const UI *ui);

// Destroy UI
void ui_destroy(UI *ui);

#endif // UI_H