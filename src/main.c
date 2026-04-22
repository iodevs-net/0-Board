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

static void error_exit(const char *message) {
    fprintf(stderr, "Error: %s\n", message);
    exit(1);
}

int main() {
    Config config;
    config_load_defaults(&config);

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
        .max_fonts_to_cache = 50
    };

    FontManager *font_manager = font_manager_create(&font_config);
    if (!font_manager) error_exit("Failed to create font manager");

    // 4. Window size from screen
    Display *tmp_dpy = XOpenDisplay(NULL);
    if (!tmp_dpy) error_exit("Cannot open X11 display");
    int screen_w = DisplayWidth(tmp_dpy, DefaultScreen(tmp_dpy));
    XCloseDisplay(tmp_dpy);

    double size_ratios[] = {
        SCREEN_WIDTH_RATIO_SMALL,
        SCREEN_WIDTH_RATIO_MEDIUM,
        SCREEN_WIDTH_RATIO_LARGE
    };
    int kb_width = screen_w * size_ratios[config.keyboard_size];
    int kb_height = kb_width * KEYBOARD_HEIGHT_RATIO;

    // 5. X11 window
    WindowConfig window_config = {
        .x = -1, .y = -1,
        .width = kb_width,
        .height = kb_height,
        .title = "0-board",
        .borderless = config.window_borderless,
        .override_redirect = false,
        .skip_taskbar = config.window_skip_taskbar,
        .opacity = config.window_opacity
    };

    X11Window *window = x11_window_create(&window_config);
    if (!window) error_exit("Failed to create X11 window");

    // 6. Renderer
    RendererConfig renderer_config = {
        .width = window_config.width,
        .height = window_config.height,
        .default_opacity = 1.0,
        .clear_color = {0.0, 0.0, 0.0, 0.0}
    };

    Renderer *renderer = renderer_create_for_x11_window(window, &renderer_config);
    if (!renderer) error_exit("Failed to create renderer");

    // 7. Engine
    EngineConfig engine_config = {
        .display = x11_window_get_display(window),
        .use_xtest = true,
        .event_delay_ms = 0
    };

    Engine *engine = engine_create(&engine_config);
    if (!engine) error_exit("Failed to create engine");

    // 8. UI
    UIConfig ui_config = {
        .title = "0-board",
        .initial_opacity = config.window_opacity,
        .show_menu_bar = false,
        .initial_size = config.keyboard_size
    };

    UI *ui = ui_create(&ui_config, keyboard, renderer, window, font_manager);
    if (!ui) error_exit("Failed to create UI");
    ui_set_engine(ui, engine);

    printf("0-board ready.\n");

    // 9. Main loop
    x11_window_show(window);
    ui_run(ui);

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
