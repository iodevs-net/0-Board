/*
 * 0-Board Virtual Keyboard
 * Copyright (c) 2026 Leonardo Vergara <leonardovergaramarin@gmail.com>
 * Licensed under the MIT License.
 */
#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H

#include <stdbool.h>

// Forward declaration
typedef struct FontManager FontManager;

// Font configuration
typedef struct {
    const char *preferred_family;  // Preferred font family (NULL for default)
    bool load_all_system_fonts;    // Whether to scan all system fonts at startup
    int max_fonts_to_cache;        // Maximum fonts to keep in cache
    const char *font_dir;            // Custom font directory path (NULL for default "./assets/fonts")
} FontConfig;

// Create a new font manager
FontManager* font_manager_create(FontConfig *config);

// Get the current font family
const char* font_manager_get_current_family(FontManager *fm);

// Get the next font family in the list
const char* font_manager_get_next_family(FontManager *fm);

// Get the previous font family in the list  
const char* font_manager_get_previous_family(FontManager *fm);

// Get the number of available fonts
int font_manager_get_font_count(FontManager *fm);

// Check if a specific font family exists
bool font_manager_has_family(FontManager *fm, const char *family);

// Set the current font family by name
bool font_manager_set_family(FontManager *fm, const char *family);

// Destroy the font manager
void font_manager_destroy(FontManager *fm);

#endif // FONT_MANAGER_H