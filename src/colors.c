// SPDX-License-Identifier: MIT — see LICENSE file

#include "colors.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Hex helper: (r, g, b, a) where r/g/b are 0-255 mapped to 0.0-1.0
#define RGBA(r, g, b, a) {(r)/255.0, (g)/255.0, (b)/255.0, (a)}

static Color parse_hex(const char *hex) {
    if (!hex) return (Color){0,0,0,1};
    if (hex[0] == '#') hex++;
    
    unsigned int r = 0, g = 0, b = 0;
    if (strlen(hex) == 6) {
        sscanf(hex, "%02x%02x%02x", &r, &g, &b);
    }
    return (Color){r/255.0, g/255.0, b/255.0, 1.0};
}

ColorScheme color_scheme_get(int id) {
    switch (id) {
        case COLOR_SCHEME_SPACE_GRAY:
            // "Macbook Pro / Magic Keyboard" aesthetic
            return (ColorScheme){
                .background    = RGBA(45, 45, 48,    1.0), // Space Grey Aluminum Chassis
                .text_primary  = RGBA(210, 210, 215, 1.0), // Crisp but slightly muted white
                .text_secondary= RGBA(150, 150, 155, 1.0), // Menu text
                .key_normal    = RGBA(22, 22, 24,    1.0), // Deep black keys
                .key_pressed   = RGBA(60, 60, 65,    1.0), // Highlight when pressed
                .key_modifier  = RGBA(22, 22, 24,    1.0), // Uniform color (no grey modifiers)
                .key_text      = RGBA(22, 22, 24,    1.0), // Uniform
                .key_number    = RGBA(22, 22, 24,    1.0), // Uniform
                .key_special   = RGBA(22, 22, 24,    1.0), // Uniform
                .shift_active  = RGBA(255, 255, 255, 0.15),
                .accent        = RGBA(10, 132, 255,  1.0),
                .drag_handle   = RGBA(0, 0, 0,       0.5),
                .key_shadow    = RGBA(0, 0, 0,       0.4), // Subtle shadow to ground the keys
            };

        case COLOR_SCHEME_SILVER:
            return (ColorScheme){
                .background    = RGBA(199, 199, 204, 1.0),
                .text_primary  = RGBA(0,   0,   0,   1.0),
                .text_secondary= RGBA(99,  99, 102,  1.0),
                .key_normal    = RGBA(255, 255, 255, 1.0),
                .key_pressed   = RGBA(188, 188, 192, 1.0),
                .key_modifier  = RGBA(172, 177, 185, 1.0),
                .key_text      = RGBA(255, 255, 255, 1.0),
                .key_number    = RGBA(255, 255, 255, 1.0),
                .key_special   = RGBA(220, 220, 224, 1.0),
                .shift_active  = RGBA(0, 122, 255,   1.0),
                .accent        = RGBA(0, 122, 255,   1.0),
                .drag_handle   = RGBA(142, 142, 147, 0.4),
                .key_shadow    = RGBA(0, 0, 0,       0.12),
            };

        default:
            return color_scheme_get(COLOR_SCHEME_SPACE_GRAY);
    }
}

ColorScheme color_scheme_load_custom(const char *path) {
    ColorScheme s = color_scheme_get(COLOR_SCHEME_SPACE_GRAY);
    if (!path) return s;

    FILE *f = fopen(path, "r");
    if (!f) return s;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char key[64], val[64];
        if (sscanf(line, "%63[^= ] = %63s", key, val) == 2) {
            if (strcmp(key, "background") == 0) s.background = parse_hex(val);
            else if (strcmp(key, "text") == 0) s.text_primary = parse_hex(val);
            else if (strcmp(key, "text_secondary") == 0) s.text_secondary = parse_hex(val);
            else if (strcmp(key, "key_normal") == 0) s.key_normal = parse_hex(val);
            else if (strcmp(key, "key_pressed") == 0) s.key_pressed = parse_hex(val);
            else if (strcmp(key, "key_modifier") == 0) s.key_modifier = parse_hex(val);
            else if (strcmp(key, "key_text") == 0) s.key_text = parse_hex(val);
            else if (strcmp(key, "key_number") == 0) s.key_number = parse_hex(val);
            else if (strcmp(key, "key_special") == 0) s.key_special = parse_hex(val);
            else if (strcmp(key, "shift_active") == 0) s.shift_active = parse_hex(val);
            else if (strcmp(key, "accent") == 0) s.accent = parse_hex(val);
            else if (strcmp(key, "drag_handle") == 0) s.drag_handle = parse_hex(val);
        }
    }

    fclose(f);
    return s;
}