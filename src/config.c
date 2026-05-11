// SPDX-License-Identifier: MIT — see LICENSE file

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

void config_load_defaults(Config *config) {
    if (!config) return;

    config->window_width = 800;
    config->window_height = 360;
    config->window_opacity = 0.94;
    config->window_borderless = true;
    config->window_skip_taskbar = true;

    config->keyboard_size = 1; // Medium
    config->color_scheme = 1;  // Dark
    config->show_menu_bar = false;

    config->double_buffering = true;
    config->lazy_font_loading = true;

    config->key_event_delay_us = DEFAULT_KEY_EVENT_DELAY_US;
    strncpy(config->voice_recording_flag, DEFAULT_VOICE_RECORDING_FLAG,
            sizeof(config->voice_recording_flag) - 1);
    config->voice_recording_flag[sizeof(config->voice_recording_flag) - 1] = '\0';
    strncpy(config->voice_script_path, DEFAULT_VOICE_SCRIPT_PATH,
            sizeof(config->voice_script_path) - 1);
    config->voice_script_path[sizeof(config->voice_script_path) - 1] = '\0';
    // Resolve font directory from HOME
    const char *home = getenv("HOME");
    if (home) {
        snprintf(config->font_dir, sizeof(config->font_dir),
                 "%s/0-Board/assets/fonts", home);
    } else {
        strncpy(config->font_dir, "./assets/fonts", sizeof(config->font_dir) - 1);
        config->font_dir[sizeof(config->font_dir) - 1] = '\0';
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
        
        // Match key and set value
        if (strcmp(key, "window_width") == 0) {
            config->window_width = atoi(value);
        } else if (strcmp(key, "window_height") == 0) {
            config->window_height = atoi(value);
        } else if (strcmp(key, "window_opacity") == 0) {
            config->window_opacity = atof(value);
        } else if (strcmp(key, "window_borderless") == 0) {
            config->window_borderless = (strcmp(value, "true") == 0);
        } else if (strcmp(key, "window_skip_taskbar") == 0) {
            config->window_skip_taskbar = (strcmp(value, "true") == 0);
        } else if (strcmp(key, "keyboard_size") == 0) {
            config->keyboard_size = atoi(value);
        } else if (strcmp(key, "color_scheme") == 0) {
            config->color_scheme = atoi(value);
        } else if (strcmp(key, "show_menu_bar") == 0) {
            config->show_menu_bar = (strcmp(value, "true") == 0);
        } else if (strcmp(key, "double_buffering") == 0) {
            config->double_buffering = (strcmp(value, "true") == 0);
        } else if (strcmp(key, "lazy_font_loading") == 0) {
            config->lazy_font_loading = (strcmp(value, "true") == 0);
        } else if (strcmp(key, "key_event_delay_us") == 0) {
            config->key_event_delay_us = atoi(value);
        } else if (strcmp(key, "voice_recording_flag") == 0) {
            strncpy(config->voice_recording_flag, value,
                    sizeof(config->voice_recording_flag) - 1);
            config->voice_recording_flag[sizeof(config->voice_recording_flag) - 1] = '\0';
        } else if (strcmp(key, "voice_script_path") == 0) {
            strncpy(config->voice_script_path, value,
                    sizeof(config->voice_script_path) - 1);
            config->voice_script_path[sizeof(config->voice_script_path) - 1] = '\0';
        } else if (strcmp(key, "font_dir") == 0) {
            strncpy(config->font_dir, value,
                    sizeof(config->font_dir) - 1);
            config->font_dir[sizeof(config->font_dir) - 1] = '\0';
        }
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
    
    fprintf(f, "window_width = %d\n", config->window_width);
    fprintf(f, "window_height = %d\n", config->window_height);
    fprintf(f, "window_opacity = %.2f\n", config->window_opacity);
    fprintf(f, "window_borderless = %s\n", config->window_borderless ? "true" : "false");
    fprintf(f, "window_skip_taskbar = %s\n", config->window_skip_taskbar ? "true" : "false");
    fprintf(f, "\n");
    fprintf(f, "keyboard_size = %d\n", config->keyboard_size);
    fprintf(f, "color_scheme = %d\n", config->color_scheme);
    fprintf(f, "show_menu_bar = %s\n", config->show_menu_bar ? "true" : "false");
    fprintf(f, "\n");
    fprintf(f, "double_buffering = %s\n", config->double_buffering ? "true" : "false");
    fprintf(f, "lazy_font_loading = %s\n", config->lazy_font_loading ? "true" : "false");
    fprintf(f, "key_event_delay_us = %d\n", config->key_event_delay_us);
    fprintf(f, "\n");
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