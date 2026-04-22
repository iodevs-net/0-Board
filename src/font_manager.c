#include "font_manager.h"

#define FM_DEFAULT_CACHE_SIZE 50
#include <fontconfig/fontconfig.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

struct FontManager {
    char **fonts;
    int font_count;
    int capacity;
    int current_index;
    bool fonts_loaded;
    FontConfig config;
};

// Common sans-serif fonts to try first
static const char* DEFAULT_FONT_FAMILIES[] = {
    "Inter",
    "DejaVu Sans",
    "Liberation Sans", 
    "Roboto",
    "Ubuntu",
    "Noto Sans",
    "FreeSans",
    "Arial",
    "Helvetica",
    NULL // Sentinel
};

static bool case_insensitive_contains(const char *haystack, const char *needle) {
    if (!haystack || !needle) return false;

    const char *h = haystack;
    const char *n = needle;

    while (*h) {
        if (tolower(*h) == tolower(*n)) {
            const char *h2 = h;
            const char *n2 = n;

            while (*h2 && *n2 && tolower(*h2) == tolower(*n2)) {
                h2++;
                n2++;
            }

            if (!*n2) {
                return true;
            }
        }
        h++;
    }

    return false;
}

static void load_fonts(FontManager *fm) {
    FcConfig *cfg = FcInitLoadConfigAndFonts();
    if (!cfg) {
        fprintf(stderr, "font_manager: failed to initialize FontConfig\n");
        goto fallback;
    }

    // Register local fonts and MAKE THIS CONFIG CURRENT
    const char *local_fonts_path = "./assets/fonts";
    if (FcConfigAppFontAddDir(cfg, (const FcChar8*)local_fonts_path)) {
        printf("font_manager: applying local font config from %s\n", local_fonts_path);
        FcConfigSetCurrent(cfg);
    }
    
    FcPattern *pat = FcPatternCreate();
    FcObjectSet *os = FcObjectSetBuild(FC_FAMILY, NULL);
    FcFontSet *fs = FcFontList(cfg, pat, os);
    
    if (fs) {
        // Allocate initial space
        fm->capacity = fm->config.max_fonts_to_cache;
        fm->fonts = malloc(sizeof(char*) * fm->capacity);
        if (!fm->fonts) {
            fprintf(stderr, "font_manager: failed to allocate font array\n");
            FcFontSetDestroy(fs);
            goto cleanup;
        }
        
        // Scan fonts
        for (int i = 0; i < fs->nfont && fm->font_count < fm->capacity; i++) {
            FcChar8 *fam;
            if (FcPatternGetString(fs->fonts[i], FC_FAMILY, 0, &fam) != FcResultMatch) {
                continue;
            }
            
            // Check for duplicates
            bool duplicate = false;
            for (int j = 0; j < fm->font_count; j++) {
                if (strcmp(fm->fonts[j], (char*)fam) == 0) {
                    duplicate = true;
                    break;
                }
            }
            
            if (!duplicate) {
                fm->fonts[fm->font_count] = strdup((char*)fam);
                if (fm->fonts[fm->font_count]) {
                    fm->font_count++;
                }
            }
        }
        
        FcFontSetDestroy(fs);
    }
    
cleanup:
    FcObjectSetDestroy(os);
    FcPatternDestroy(pat);
    FcConfigDestroy(cfg);
    
fallback:
    // Add default fonts if we didn't load any
    if (fm->font_count == 0) {
        fm->capacity = 8;
        fm->fonts = malloc(sizeof(char*) * fm->capacity);
        if (fm->fonts) {
            for (int i = 0; DEFAULT_FONT_FAMILIES[i] && fm->font_count < fm->capacity; i++) {
                fm->fonts[fm->font_count] = strdup(DEFAULT_FONT_FAMILIES[i]);
                if (fm->fonts[fm->font_count]) {
                    fm->font_count++;
                }
            }
        }
    }
    
    fm->fonts_loaded = true;

    // Set initial font
    if (fm->config.preferred_family) {
        font_manager_set_family(fm, fm->config.preferred_family);
    } else {
        // Try to find a sans-serif font
        for (int i = 0; i < fm->font_count; i++) {
            for (int j = 0; DEFAULT_FONT_FAMILIES[j]; j++) {
                if (case_insensitive_contains(fm->fonts[i], DEFAULT_FONT_FAMILIES[j])) {
                    fm->current_index = i;
                    break;
                }
            }
            if (fm->current_index >= 0) break;
        }
        
        // Fallback to first font
        if (fm->current_index < 0 && fm->font_count > 0) {
            fm->current_index = 0;
        }
    }
}

FontManager* font_manager_create(FontConfig *config) {
    FontManager *fm = malloc(sizeof(FontManager));
    if (!fm) {
        return NULL;
    }
    
    fm->fonts = NULL;
    fm->font_count = 0;
    fm->capacity = 0;
    fm->current_index = -1;
    fm->fonts_loaded = false;
    
    if (config) {
        fm->config = *config;
    } else {
        fm->config.preferred_family = NULL;
        fm->config.load_all_system_fonts = true;
        fm->config.max_fonts_to_cache = FM_DEFAULT_CACHE_SIZE;
    }
    
    // Load fonts immediately if configured to do so
    if (fm->config.load_all_system_fonts) {
        load_fonts(fm);
    }
    
    return fm;
}

const char* font_manager_get_current_family(FontManager *fm) {
    if (!fm) return NULL;
    
    if (!fm->fonts_loaded) {
        load_fonts(fm);
    }
    
    if (fm->current_index >= 0 && fm->current_index < fm->font_count) {
        return fm->fonts[fm->current_index];
    }
    
    return "DejaVu Sans"; // Ultimate fallback
}

const char* font_manager_get_next_family(FontManager *fm) {
    if (!fm || fm->font_count == 0) return NULL;
    
    if (!fm->fonts_loaded) {
        load_fonts(fm);
    }
    
    fm->current_index = (fm->current_index + 1) % fm->font_count;
    return font_manager_get_current_family(fm);
}

const char* font_manager_get_previous_family(FontManager *fm) {
    if (!fm || fm->font_count == 0) return NULL;
    
    if (!fm->fonts_loaded) {
        load_fonts(fm);
    }
    
    fm->current_index = (fm->current_index - 1 + fm->font_count) % fm->font_count;
    return font_manager_get_current_family(fm);
}

int font_manager_get_font_count(FontManager *fm) {
    if (!fm) return 0;
    
    if (!fm->fonts_loaded) {
        load_fonts(fm);
    }
    
    return fm->font_count;
}

bool font_manager_has_family(FontManager *fm, const char *family) {
    if (!fm || !family) return false;
    
    if (!fm->fonts_loaded) {
        load_fonts(fm);
    }
    
    for (int i = 0; i < fm->font_count; i++) {
        if (strcasecmp(fm->fonts[i], family) == 0) {
            return true;
        }
    }
    
    return false;
}

bool font_manager_set_family(FontManager *fm, const char *family) {
    if (!fm || !family) return false;
    
    if (!fm->fonts_loaded) {
        load_fonts(fm);
    }
    
    for (int i = 0; i < fm->font_count; i++) {
        if (strcasecmp(fm->fonts[i], family) == 0) {
            fm->current_index = i;
            return true;
        }
    }
    
    return false;
}

void font_manager_destroy(FontManager *fm) {
    if (!fm) return;
    
    if (fm->fonts) {
        for (int i = 0; i < fm->font_count; i++) {
            free(fm->fonts[i]);
        }
        free(fm->fonts);
    }
    
    free(fm);
}