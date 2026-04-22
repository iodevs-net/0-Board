#include "colors.h"

// Hex helper: (r, g, b, a) where r/g/b are 0-255 mapped to 0.0-1.0
#define RGBA(r, g, b, a) {(r)/255.0, (g)/255.0, (b)/255.0, (a)}

ColorScheme color_scheme_get(int id) {
    switch (id) {
        case COLOR_SCHEME_SPACE_GRAY:
            return (ColorScheme){
                .background    = RGBA(28, 28, 30,    1.0),   // #1c1c1e
                .key_normal    = RGBA(58, 58, 60,    1.0),   // #3a3a3c
                .key_modifier  = RGBA(44, 44, 46,    1.0),   // #2c2c2e
                .key_special   = RGBA(72, 72, 74,    1.0),   // #48484a
                .text_primary  = RGBA(255, 255, 255, 1.0),
                .text_secondary= RGBA(174, 174, 178, 1.0),   // #aeaeb2
                .key_pressed   = RGBA(99, 99, 102,   1.0),   // #636366
                .accent        = RGBA(10, 132, 255,  1.0),   // #0a84ff (iOS blue)
                .drag_handle   = RGBA(142, 142, 147, 0.5),   // #8e8e93
                .key_shadow    = RGBA(0, 0, 0,       0.30),
            };

        case COLOR_SCHEME_SILVER:
            return (ColorScheme){
                .background    = RGBA(199, 199, 204, 1.0),   // #c7c7cc
                .key_normal    = RGBA(255, 255, 255, 1.0),   // #ffffff
                .key_modifier  = RGBA(172, 177, 185, 1.0),   // #acb1b9
                .key_special   = RGBA(220, 220, 224, 1.0),   // #dcdce0
                .text_primary  = RGBA(0,   0,   0,   1.0),
                .text_secondary= RGBA(99,  99, 102,  1.0),   // #636366
                .key_pressed   = RGBA(188, 188, 192, 1.0),   // #bcbcc0
                .accent        = RGBA(0, 122, 255,   1.0),   // #007aff
                .drag_handle   = RGBA(142, 142, 147, 0.4),   // #8e8e93
                .key_shadow    = RGBA(0, 0, 0,       0.12),
            };

        default:
            return color_scheme_get(COLOR_SCHEME_SPACE_GRAY);
    }
}