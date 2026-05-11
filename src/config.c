// SPDX-License-Identifier: MIT — see LICENSE file

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


static bool is_true(const char *s) {
    return s && strcmp(s, "true") == 0;
}

static void set_str(char *dst, size_t max, const char *val) {
    if (val) {
        strncpy(dst, val, max - 1);
        dst[max - 1] = '\0';
    }
}

void config_load_defaults(Config *config) {
    if (!config) return;

    // Set defaults via X macros
    #define X_DEFAULT(field, def) config->field = def;
    CONFIG_FIELDS_INT(X_DEFAULT)
    CONFIG_FIELDS_DOUBLE(X_DEFAULT)
    CONFIG_FIELDS_BOOL(X_DEFAULT)
    #undef X_DEFAULT

    // String defaults via set_str
    #define X_DEFAULT(field, def) set_str(config->field, sizeof(config->field), def);
    CONFIG_FIELDS_STR(X_DEFAULT)
    #undef X_DEFAULT

    // Resolve font directory from HOME (overrides the empty default)
    const char *home = getenv("HOME");
    if (home) {
        snprintf(config->font_dir, sizeof(config->font_dir),
                 "%s/0-Board/assets/fonts", home);
    } else {
        strncpy(config->font_dir, "./assets/fonts", sizeof(config->font_dir) - 1);
        config->font_dir[sizeof(config->font_dir) - 1] = '\0';
    }
    // Also check if installed via make install
    char install_fonts[512];
    if (home) {
        snprintf(install_fonts, sizeof(install_fonts), "%s/.local/share/0-board/fonts", home);
        if (access(install_fonts, F_OK) == 0) {
            strncpy(config->font_dir, install_fonts, sizeof(config->font_dir) - 1);
            config->font_dir[sizeof(config->font_dir) - 1] = '\0';
        }
    }
}

static void trim(char *str) {
    if (!str) return;
    
    // Remove trailing newline/carriage return
    size_t len = strlen(str);
    while (len > 0 && (str[len-1] == '\n' || str[len-1] == '\r')) {
        str[len-1] = '\0';
        len--;
    }
    
    // Remove leading spaces
    size_t start = 0;
    while (str[start] == ' ' || str[start] == '\t') start++;
    if (start > 0) {
        memmove(str, str + start, len - start + 1);
    }
    
    // Remove trailing spaces
    len = strlen(str);
    while (len > 0 && (str[len-1] == ' ' || str[len-1] == '\t')) {
        str[len-1] = '\0';
        len--;
    }
}

bool config_load_from_file(Config *config, const char *filename) {
    if (!config || !filename) return false;
    
    FILE *f = fopen(filename, "r");
    if (!f) {
        return false;
    }
    
    // Start with defaults
    config_load_defaults(config);
    
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        trim(line);
        
        // Skip empty lines and comments
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        
        // Parse key = value
        char *sep = strchr(line, '=');
        if (!sep) continue;
        
        *sep = '\0';
        char *key = line;
        char *value = sep + 1;
        
        trim(key);
        trim(value);
        
        // Match key and set value using X macro dispatch
        if (0) {}
        #define X_PARSE(field, def) else if (strcmp(key, #field) == 0) config->field = atoi(value);
        CONFIG_FIELDS_INT(X_PARSE)
        #undef X_PARSE
        #define X_PARSE(field, def) else if (strcmp(key, #field) == 0) config->field = atof(value);
        CONFIG_FIELDS_DOUBLE(X_PARSE)
        #undef X_PARSE
        #define X_PARSE(field, def) else if (strcmp(key, #field) == 0) config->field = is_true(value);
        CONFIG_FIELDS_BOOL(X_PARSE)
        #undef X_PARSE
        // String fields handled explicitly (char[] needs set_str)
        else if (strcmp(key, "voice_recording_flag") == 0)
            set_str(config->voice_recording_flag, sizeof(config->voice_recording_flag), value);
        else if (strcmp(key, "voice_script_path") == 0)
            set_str(config->voice_script_path, sizeof(config->voice_script_path), value);
        else if (strcmp(key, "font_dir") == 0)
            set_str(config->font_dir, sizeof(config->font_dir), value);
        // Unknown keys are ignored
    }
    
    fclose(f);
    return true;
}

bool config_save_to_file(const Config *config, const char *filename) {
    if (!config || !filename) return false;
    
    FILE *f = fopen(filename, "w");
    if (!f) {
        return false;
    }
    
    
    fprintf(f, "# 0-board configuration\n");
    fprintf(f, "# Generated automatically\n\n");
    
    #define X_SAVE(field, def) fprintf(f, #field " = %d\n", config->field);
    CONFIG_FIELDS_INT(X_SAVE)
    #undef X_SAVE
    
    #define X_SAVE(field, def) fprintf(f, #field " = %.2f\n", config->field);
    CONFIG_FIELDS_DOUBLE(X_SAVE)
    #undef X_SAVE
    
    #define X_SAVE(field, def) fprintf(f, #field " = %s\n", config->field ? "true" : "false");
    CONFIG_FIELDS_BOOL(X_SAVE)
    #undef X_SAVE
    
    // String fields are still hardcoded (char[] fields don't macro well with %s)
    fprintf(f, "voice_recording_flag = %s\n", config->voice_recording_flag);
    fprintf(f, "voice_script_path = %s\n", config->voice_script_path);
    fprintf(f, "font_dir = %s\n", config->font_dir);

    fclose(f);
    return true;
}

char* config_get_default_path(void) {
    const char *home = getenv("HOME");
    if (!home) return NULL;
    
    // Create ~/.config/0-board directory if it doesn't exist
    char dir_path[1024];
    snprintf(dir_path, sizeof(dir_path), "%s/.config/0-board", home);
    
    // We don't create it here, just return the file path
    char *file_path = malloc(strlen(dir_path) + 12); // /config.ini
    if (!file_path) return NULL;
    
    sprintf(file_path, "%s/config.ini", dir_path);
    return file_path;
}