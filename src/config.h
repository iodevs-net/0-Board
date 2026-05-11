/*
 * 0-Board Virtual Keyboard
 * Copyright (c) 2026 Leonardo Vergara <leonardovergaramarin@gmail.com>
 * Licensed under the MIT License.
 */
#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

// Configuration structure for 0-board
typedef struct {
    // Window settings
    int window_width;
    int window_height;
    double window_opacity;
    bool window_borderless;
    bool window_skip_taskbar;

    // Keyboard settings
    int keyboard_size; // 0=small, 1=medium, 2=large
    int color_scheme;  // 0=light, 1=dark, 2=auto
    bool show_menu_bar;

    // Advanced settings
    bool double_buffering;
    bool lazy_font_loading;

    // Key event timing (microseconds delay between press/release)
    int key_event_delay_us;

    // Voice feature paths
    char voice_recording_flag[128];  // Path to recording flag file
    char voice_script_path[256];     // Path to voice script binary
} Config;

#define DEFAULT_VOICE_RECORDING_FLAG "/tmp/0-voice-recording"
#define DEFAULT_VOICE_SCRIPT_PATH   "/usr/local/bin/0-voice"
#define DEFAULT_KEY_EVENT_DELAY_US  10000

// Load default configuration
void config_load_defaults(Config *config);

// Load configuration from file
// Returns true on success, false on failure (falls back to defaults)
bool config_load_from_file(Config *config, const char *filename);

// Save configuration to file
// Returns true on success, false on failure
bool config_save_to_file(const Config *config, const char *filename);

// Get the default config file path (caller must free)
char* config_get_default_path(void);

#endif // CONFIG_H