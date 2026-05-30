// SPDX-License-Identifier: MIT — see LICENSE file

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

static bool is_duplicate(FontManager *fm, const char *name) {
    for (int i = 0; i < fm->font_count; i++)
        if (strcmp(fm->fonts[i], name) == 0) return true;
    return false;
}

static void load_system_fonts(FontManager *fm, FcConfig *cfg) {
    FcPattern *pat = FcPatternCreate();
    FcObjectSet *os = FcObjectSetBuild(FC_FAMILY, NULL);
    FcFontSet *fs = FcFontList(cfg, pat, os);

    if (!fs) { FcObjectSetDestroy(os); FcPatternDestroy(pat); return; }

    for (int i = 0; i < fs->nfont && fm->font_count < fm->capacity; i++) {
        FcChar8 *fam;
        if (FcPatternGetString(fs->fonts[i], FC_FAMILY, 0, &fam) != FcResultMatch)
            continue;
        if (is_duplicate(fm, (const char*)fam)) continue;
        fm->fonts[fm->font_count++] = strdup((const char*)fam);
    }

    FcFontSetDestroy(fs);
    FcObjectSetDestroy(os);
    FcPatternDestroy(pat);
}

static void load_local_fonts(FontManager *fm, FcConfig *cfg) {
    if (!fm->config.font_dir) return;
    FcConfigAppFontAddDir(cfg, (const FcChar8*)fm->config.font_dir);
    FcConfigSetCurrent(cfg);
    printf("font_manager: loaded local fonts from %s\n", fm->config.font_dir);
}

static void load_fallback_fonts(FontManager *fm) {
    fm->capacity = 8;
    fm->fonts = malloc(sizeof(char*) * fm->capacity);
    if (!fm->fonts) return;
    for (int i = 0; DEFAULT_FONT_FAMILIES[i] && fm->font_count < fm->capacity; i++)
        fm->fonts[fm->font_count++] = strdup(DEFAULT_FONT_FAMILIES[i]);
}

static void select_initial_font(FontManager *fm) {
    if (!fm->font_count) { fm->current_index = -1; return; }

    if (fm->config.preferred_family) {
        for (int i = 0; i < fm->font_count; i++) {
            if (strcasecmp(fm->fonts[i], fm->config.preferred_family) == 0) {
                fm->current_index = i;
                return;
            }
        }
    }

    // Try to find a known sans-serif font
    for (int i = 0; i < fm->font_count; i++)
        for (int j = 0; DEFAULT_FONT_FAMILIES[j]; j++)
            if (case_insensitive_contains(fm->fonts[i], DEFAULT_FONT_FAMILIES[j]))
                { fm->current_index = i; return; }

    fm->current_index = 0;
}

static void ensure_fonts_loaded(FontManager *fm) {
    if (fm->fonts_loaded) return;

    FcConfig *cfg = FcInitLoadConfigAndFonts();
    if (!cfg) {
        load_fallback_fonts(fm);
        fm->fonts_loaded = true;
        select_initial_font(fm);
        return;
    }

    load_local_fonts(fm, cfg);

    fm->capacity = fm->config.max_fonts_to_cache;
    fm->fonts = malloc(sizeof(char*) * fm->capacity);
    if (fm->fonts) {
        load_system_fonts(fm, cfg);
    }

    FcConfigDestroy(cfg);

    if (fm->font_count == 0) {
        if (fm->fonts) {
            free(fm->fonts);
            fm->fonts = NULL;
        }
        load_fallback_fonts(fm);
    }

    fm->fonts_loaded = true;
    select_initial_font(fm);
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
        ensure_fonts_loaded(fm);
    }
    
    return fm;
}

const char* font_manager_get_current_family(FontManager *fm) {
    if (!fm) return NULL;
    
    if (!fm->fonts_loaded) {
        ensure_fonts_loaded(fm);
    }
    
    if (fm->current_index >= 0 && fm->current_index < fm->font_count) {
        return fm->fonts[fm->current_index];
    }
    
    return "DejaVu Sans"; // Ultimate fallback
}

const char* font_manager_get_next_family(FontManager *fm) {
    if (!fm || fm->font_count == 0) return NULL;
    
    if (!fm->fonts_loaded) {
        ensure_fonts_loaded(fm);
    }
    
    fm->current_index = (fm->current_index + 1) % fm->font_count;
    return font_manager_get_current_family(fm);
}

const char* font_manager_get_previous_family(FontManager *fm) {
    if (!fm || fm->font_count == 0) return NULL;
    
    if (!fm->fonts_loaded) {
        ensure_fonts_loaded(fm);
    }
    
    fm->current_index = (fm->current_index - 1 + fm->font_count) % fm->font_count;
    return font_manager_get_current_family(fm);
}

int font_manager_get_font_count(FontManager *fm) {
    if (!fm) return 0;
    
    if (!fm->fonts_loaded) {
        ensure_fonts_loaded(fm);
    }
    
    return fm->font_count;
}

bool font_manager_has_family(FontManager *fm, const char *family) {
    if (!fm || !family) return false;
    
    if (!fm->fonts_loaded) {
        ensure_fonts_loaded(fm);
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
        ensure_fonts_loaded(fm);
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