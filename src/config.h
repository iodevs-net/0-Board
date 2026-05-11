// SPDX-License-Identifier: MIT — see LICENSE file

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
    char font_dir[512];              // Path to assets/fonts directory (resolved from HOME)
} Config;

#define DEFAULT_VOICE_RECORDING_FLAG "/tmp/0-voice-recording"
#define DEFAULT_VOICE_SCRIPT_PATH   "/usr/local/bin/0-voice"
#define DEFAULT_KEY_EVENT_DELAY_US  10000


#define CONFIG_FIELDS_INT(X) \
    X(window_width, 800) \
    X(window_height, 360) \
    X(keyboard_size, 1) \
    X(color_scheme, 1) \
    X(key_event_delay_us, 10000)

#define CONFIG_FIELDS_DOUBLE(X) \
    X(window_opacity, 0.94)

#define CONFIG_FIELDS_BOOL(X) \
    X(window_borderless, true) \
    X(window_skip_taskbar, true) \
    X(show_menu_bar, false) \
    X(double_buffering, true) \
    X(lazy_font_loading, true)

#define CONFIG_FIELDS_STR(X) \
    X(voice_recording_flag, "/tmp/0-voice-recording") \
    X(voice_script_path, "/usr/local/bin/0-voice") \
    X(font_dir, "")

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