/*
 * 0-Board Virtual Keyboard
 * Copyright (c) 2026 Leonardo Vergara <leonardovergaramarin@gmail.com>
 * Licensed under the MIT License.
 */
#include "keyboard.h"
#include "layout.h"
#include "x11_window.h"
#include "x11_cairo_bridge.h"
#include "config.h"
#include "font_manager.h"
#include "engine.h"
#include "ui.h"
#include "colors.h"
#include "constants.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

static volatile sig_atomic_t g_shutdown = 0;
volatile sig_atomic_t g_toggle_visibility = 0;

static void signal_handler(int sig) {
    if (sig == SIGUSR1) {
        g_toggle_visibility = 1;
    } else {
        g_shutdown = 1;
    }
}

static void error_exit(const char *message) {
    fprintf(stderr, "Error: %s\n", message);
    exit(1);
}

int main() {
    // Catch SIGTERM, SIGINT, SIGHUP for clean exit
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);

    Config config;
    config_load_defaults(&config);
    // Load config from file (overrides defaults)
    char *cfg_path = config_get_default_path();
    if (cfg_path) {
        config_load_from_file(&config, cfg_path);
        free(cfg_path);
    }

    DEBUG_INIT("debug.log");
    LOG_INFO("Starting 0-board...");
    printf("Starting 0-board...\n");

    // 1. Layout
    Layout *layout = layout_get_default();
    if (!layout) error_exit("Failed to get default layout");
    layout_init(layout);

    // 2. Keyboard
    Keyboard *keyboard = keyboard_create(layout);
    if (!keyboard) error_exit("Failed to create keyboard");

    // 3. Font manager
    FontConfig font_config = {
        .preferred_family = "Inter",
        .load_all_system_fonts = false,
        .max_fonts_to_cache = 50,
        .font_dir = config.font_dir,
    };

    FontManager *font_manager = font_manager_create(&font_config);
    if (!font_manager) error_exit("Failed to create font manager");

    // 4. X11 window (detection of screen size happens inside window module or we pass defaults)
    WindowConfig window_config = {
        .x = -1, .y = -1,
        .width = -1, // Use auto-detection based on screen size and ratio
        .height = -1,
        .title = "0-board",
        .borderless = config.window_borderless,
        .override_redirect = false,
        .skip_taskbar = config.window_skip_taskbar,
        .opacity = config.window_opacity,
        .initial_size_index = config.keyboard_size
    };

    X11Window *window = x11_window_create(&window_config);
    if (!window) error_exit("Failed to create X11 window");

    // 6. Renderer
    int actual_w, actual_h;
    x11_window_get_size(window, &actual_w, &actual_h);
    
    RendererConfig renderer_config = {
        .width = actual_w,
        .height = actual_h,
        .default_opacity = 1.0,
        .clear_color = {0.0, 0.0, 0.0, 0.0}
    };

    Renderer *renderer = renderer_create_for_x11_window(window, &renderer_config);
    if (!renderer) error_exit("Failed to create renderer");

    // 7. Engine
    EngineConfig engine_config = {
        .display = x11_window_get_display(window),
        .use_xtest = true,
        .event_delay_us = config.key_event_delay_us
    };

    Engine *engine = engine_create(&engine_config);
    if (!engine) error_exit("Failed to create engine");

    // 8. UI
    UIConfig ui_config = {
        .title = "0-board",
        .initial_opacity = config.window_opacity,
        .show_menu_bar = false,
        .initial_size = config.keyboard_size,
        .key_event_delay_us = config.key_event_delay_us,
    };
    strncpy(ui_config.voice_recording_flag, config.voice_recording_flag,
            sizeof(ui_config.voice_recording_flag) - 1);
    ui_config.voice_recording_flag[sizeof(ui_config.voice_recording_flag) - 1] = '\0';
    strncpy(ui_config.voice_script_path, config.voice_script_path,
            sizeof(ui_config.voice_script_path) - 1);
    ui_config.voice_script_path[sizeof(ui_config.voice_script_path) - 1] = '\0';

    UI *ui = ui_create(&ui_config, keyboard, renderer, window, font_manager);
    if (!ui) error_exit("Failed to create UI");
    ui_set_engine(ui, engine);

    printf("0-board ready.\n");

    // 9. Main loop — pass shutdown flag
    ui_run_with_shutdown(ui, &g_shutdown);

    // 10. Cleanup
    ui_destroy(ui);
    renderer_destroy(renderer);
    engine_destroy(engine);
    x11_window_destroy(window);
    font_manager_destroy(font_manager);
    keyboard_destroy(keyboard);

    printf("0-board shutdown.\n");
    LOG_INFO("Shutdown complete.");
    DEBUG_CLEANUP();

    return 0;
}
