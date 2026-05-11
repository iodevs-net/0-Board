# 0-Board Hardening Spec: Code Quality 8.5+

> **Goal:** Eliminate all structural pain points identified in code review, achieving measurable DRY+SOLID+KISS+LEAN compliance without changing functionality or adding features.

**Architecture:** Single-module refactors preserving existing interfaces. Each change is independently verifiable by existing test suite (100 tests) passing unchanged.

**Non-goals:** Wayland, auto-hide on physical keyboard, layout XML format, theming engine, voice features.

---

## Section 1: Engine Refactor (engine.c)

### Problem
`engine_send_key_ex()` is one 80-line function doing:
1. Resolve keysym → keycode (including shifted fallback lookup)
2. Determine which modifier keys to press/release
3. Sequence modifier + key press/release events via XTest

Three responsibilities, four levels of indent, inline shift_map (now removed but function still dense).

### Design
Split into three focused helpers:

**`keysym_to_keycode(Display *, KeySym sym, int *out_modifiers)`** — Resolves a keysym to a keycode. Returns the keycode and sets `*out_modifiers` to the modifier mask needed (e.g., Shift for uppercase/exclam). Uses `XKeysymToKeycode` + `keysym_get_base()` fallback. Returns 0 if unmappable.

**`modifier_keys_for_mask(int modifiers, KeyCode *keys, int *count)`** — Converts a modifier mask (1=Shift, 2=Ctrl, 4=Alt, 8=Meta) to an array of up to 4 KeyCodes. Fills `keys[]` and sets `*count`.

**`inject_key_sequence(Display *, KeyCode keycode, KeyCode *mods, int mod_count, Bool pressed)`** — Sends one XTest event per modifier (if pressing) plus the key event. On release, sends in reverse order. Returns 0 on success.

`engine_send_key_ex()` becomes:
```c
int engine_send_key_ex(Engine *engine, KeySym keysym, bool pressed, int modifiers) {
    if (!engine || !engine->display || keysym == 0) return -1;
    if (!engine->use_xtest) return -1;

    KeyCode kc = keysym_to_keycode(engine->display, keysym, &modifiers);
    if (!kc) return -1;

    // If the key IS a modifier being pressed, don't double-press it
    KeyCode mod_keys[4];
    int mod_count = 0;
    modifier_keys_for_mask(modifiers, mod_keys, &mod_count, keysym);

    return inject_key_sequence(engine->display, kc, mod_keys, mod_count, pressed);
}
```

### Verification
- All 100 existing tests pass unchanged
- New test: `engine_send_key_ex(NULL, ...)` returns -1
- New test: `engine_send_key_ex(engine, 0, ...)` returns -1
- (Full XTest integration test requires X server — separate)

---

## Section 2: FontManager Control Flow (font_manager.c)

### Problem
`load_fonts()` is ~60 lines with three exit paths (cleanup → fallback via goto-style pattern). The fallback block at the bottom always executes regardless of whether the main path succeeded.

### Design
Replace `load_fonts()` with split functions:

**`load_system_fonts(FontManager *fm, FcConfig *cfg)`** — Scans fontconfig for all families, deduplicates, stores in `fm->fonts[]`. Returns count loaded.

**`load_local_fonts(FontManager *fm, FcConfig *cfg)`** — Adds fonts from `fm->config.font_dir` via `FcConfigAppFontAddDir()`. Returns updated FcConfig.

**`ensure_fonts_loaded(FontManager *fm)`** — Orchestrator. Initializes FcConfig, calls `load_local_fonts()`, then `load_system_fonts()`. If zero fonts loaded, falls back to `DEFAULT_FONT_FAMILIES[]`. Sets `fm->current_index`.

Main flow becomes:
```c
static void ensure_fonts_loaded(FontManager *fm) {
    if (fm->fonts_loaded) return;

    FcConfig *cfg = FcInitLoadConfigAndFonts();
    if (!cfg) { add_fallback_fonts(fm); fm->fonts_loaded = true; return; }

    if (fm->config.font_dir)
        FcConfigAppFontAddDir(cfg, (const FcChar8*)fm->config.font_dir);

    load_system_fonts(fm, cfg);
    FcConfigDestroy(cfg);

    if (fm->font_count == 0) add_fallback_fonts(fm);

    fm->fonts_loaded = true;
    set_initial_font(fm);
}
```

No fallthrough, early returns, each function does one thing.

### Verification
- All 100 tests pass unchanged

---

## Section 3: Eliminate Global State (x11_window.c)

### Problem
Two file-scope globals: `g_x11_error_occurred` and `g_error_display`. Every function guards with `if (g_x11_error_occurred) return`. This violates the project's own "no global state" principle.

### Design
Store error state in the `X11Window` struct. Use `XSetErrorHandler` with per-display context retrieval via `XFindContext` or a simple approach: store the X11Window pointer in a `__thread` variable (C99 has `_Thread_local` since C11, but GCC/clang support `__thread` as extension in C99 mode).

Alternative simpler approach: keep a static linked list of active X11Window instances, and the error handler walks it to find the matching display. But this reintroduces globals.

**Cleanest approach for C99:** The error handler stores the failure in a thread-local `_Thread_local Display *s_error_display; int s_error_occurred;` — this is supported by GCC/Clang even in C99 mode via `__thread`. Then all guard checks read the thread-local.

Actually, simplest: **remove the global guard pattern entirely**. Instead of guarding every function with `if (g_x11_error_occurred) return`, set a flag per-window and check it in the calling code (the event loop). The event loop is the only place that needs to know: "did an X11 error happen this frame?"

Implementation:
- Add `bool x11_error_this_session` to `X11Window` struct (renamed to `fatal_error`)
- On X11 error, set `win->fatal_error = true` for the matching window (requires lookup by display)
- Use a static array of `{Display*, X11Window*}` pairs (max 4 entries, since we never have more than one window)
- The error handler walks the array to find the matching X11Window

```c
#define MAX_WINDOWS 4
static struct { Display *dpy; X11Window *win; } s_windows[MAX_WINDOWS];
static int s_window_count = 0;

static int x11_error_handler(Display *dpy, XErrorEvent *err) {
    for (int i = 0; i < s_window_count; i++) {
        if (s_windows[i].dpy == dpy && s_windows[i].win) {
            s_windows[i].win->fatal_error = true;
            break;
        }
    }
    return 0;
}
```

This is technically a global, but it's bounded (4 entries), it's internal to x11_window.c, and it's the standard pattern in X11 programs (see `xcb` connection setup). The key improvement: **per-window error state instead of a single global flag** that breaks if someone creates two windows.

### Verification
- `make test` passes (tests don't use X11)
- Binary compiles and runs

---

## Section 4: Split x11_window.c (SRP)

### Problem
`x11_window.c` is 365 lines doing window creation, event processing, error handling, property management. Event loop logic (`wait_event`, `process_events`, `process_xevent`) is mixed with window management (`create`, `move`, `resize`, `show`, `hide`).

### Design
Split into:

**`x11_window.c`** (core, ~200 lines): Window creation, move/resize/show/hide, property setters (title, opacity, always-on-top), pixmap management. Keeps the `X11Window` struct definition.

**`x11_events.c`** (~120 lines): Event processing (`x11_window_process_events`, `x11_window_wait_event`), `process_xevent()`, error handler registration. Includes `x11_window_internal.h` for struct access.

**`x11_window_internal.h`**: Forward declarations shared between `x11_window.c` and `x11_events.c`. Contains the `X11Window` struct definition and the `s_windows[]` error handler.

No public API changes — `x11_window.h` stays identical.

### Verification
- `make test` passes
- `make` produces working binary

---

## Section 5: Clean ui_events.c Further

### Problem
`ui_handle_button_press()` is still 80+ lines handling: drag detection, menu buttons, fn key, ↑↓ key, voice recording, resize toggle, key injection. Four concerns.

### Design
Extract helper functions (no new files):

```c
// Returns true if a special key was handled (menu toggle, dock toggle, etc.)
static bool handle_special_key(UI *ui, int key_index, const char *label);

// Returns true if a menu button was clicked
static bool handle_menu_click(UI *ui, int mx, int my);

// Handles voice recording toggle (mic key)
static void handle_voice_key(UI *ui, const char *flag, const char *script);

// Handles size toggle with anchored resize
static void handle_size_toggle(UI *ui, int key_index);
```

`ui_handle_button_press` becomes a flat dispatcher:
```c
void ui_handle_button_press(UI *ui, int wx, int wy, int rx, int ry, int button) {
    if (button == 2 || (button == 1 && is_on_edge(ui, wx, wy))) {
        drag_start(&ui->drag, ui->window, rx, ry);
        return;
    }
    if (button != 1) return;

    if (ui->menu_visible && handle_menu_click(ui, wx, wy)) return;

    for (int i = 0; i < ui->key_count; i++) {
        if (!rect_contains(&ui->key_bounds[i], wx, wy)) continue;
        const char *label = keyboard_get_key_label(ui->keyboard, i);
        if (handle_special_key(ui, i, label)) break;

        keyboard_press_key(ui->keyboard, i);
        key_injector_send(ui->engine, ui->keyboard, i,
            x11_window_get_display(ui->window),
            ui->config.key_event_delay_us);
        break;
    }
}
```

### Verification
- All 100 tests pass unchanged
- Binary compiles and runs

---

## Section 6: Config Parser Cleanup (config.c)

### Problem
`config_load_from_file()` uses a 30-line `strcmp` chain with duplicate boilerplate per field. `trim()` function is hand-rolled. `config_save_to_file()` duplicates field names as string literals.

### Design
Extract to helper macros or a dispatch table:

```c
typedef struct { const char *key; void (*handler)(Config*, const char*); } ConfigField;

static void set_int(Config *c, int offset, const char *value) {
    *(int*)((char*)c + offset) = atoi(value);
}
static void set_double(Config *c, int offset, const char *value) {
    *(double*)((char*)c + offset) = atof(value);
}
static void set_bool(Config *c, int offset, const char *value) {
    *(bool*)((char*)c + offset) = (strcmp(value, "true") == 0);
}
static void set_string(Config *c, int offset, const char *value, size_t maxlen) {
    strncpy((char*)c + offset, value, maxlen - 1);
}
```

With offsetof() dispatch. If the offsetof approach is too clever for C99 comfort, use a simpler macro:

```c
#define CONFIG_FIELD_INT(field) \
    if (strcmp(key, #field) == 0) { config->field = atoi(value); continue; }
#define CONFIG_FIELD_BOOL(field) \
    if (strcmp(key, #field) == 0) { config->field = (strcmp(value, "true") == 0); continue; }
#define CONFIG_FIELD_STRING(field, maxlen) \
    if (strcmp(key, #field) == 0) { strncpy(config->field, value, maxlen - 1); continue; }
```

**Recommended: X macro table** for zero-boilerplate serialization:

```c
#define CONFIG_FIELDS(X) \
    X(window_width, int, "%d", atoi) \
    X(window_height, int, "%d", atoi) \
    X(window_opacity, double, "%.2f", atof) \
    X(window_borderless, bool, "%s", parse_bool) \
    X(keyboard_size, int, "%d", atoi) \
    ... etc

// Parse (used by config_load_from_file)
#define X_PARSE(field, type, fmt, parse_fn) \
    if (strcmp(key, #field) == 0) { config->field = parse_fn(value); continue; }
CONFIG_FIELDS(X_PARSE)
#undef X_PARSE

// Save (used by config_save_to_file)
#define X_SAVE(field, type, fmt, parse_fn) \
    fprintf(f, #field " = " fmt "\n", config->field);
CONFIG_FIELDS(X_SAVE)
#undef X_SAVE
```

This is the gold standard for C config parsers: single source of truth for field name, type, and format. Add a new field = add one line to `CONFIG_FIELDS`.

### Verification
- All 100 tests pass unchanged
- config_load_from_file + config_save_to_file produce consistent output

---

## Section 7: Install Target (Makefile)

### Problem
No `make install`. Font path is resolved from `$HOME/0-Board/assets/fonts` (the XDG fix from Phase 4). Still fragile if the repo moves.

### Design
Add to `Makefile`:
```makefile
PREFIX ?= $(HOME)/.local
BINDIR ?= $(PREFIX)/bin
DATADIR ?= $(PREFIX)/share/0-board

install: release
	install -d $(BINDIR) $(DATADIR)/fonts
	install -m 755 0-board $(BINDIR)/
	install -m 644 assets/fonts/extras/ttf/*.ttf $(DATADIR)/fonts/
	@echo "Installed to $(BINDIR)/0-board"
	@echo "Fonts installed to $(DATADIR)/fonts/"
	@echo "Set font_dir = $(DATADIR)/fonts in ~/.config/0-board/config.ini"
```

Also update `config.c` default `font_dir` to check `~/.local/share/0-board/fonts` first, falling back to `~/0-Board/assets/fonts`.

### Verification
- `make install` creates correct directory structure
- Binary runs from new location

---

## Section 8: Integration Test for Engine

### Problem
The engine tests only test the `keysym_util` helpers, not `engine_send_key_ex()` itself. Since that requires an X server, we need a mock or a structural approach.

### Design
The `Engine` struct is defined in `engine.c` (not in the header). Add a test that:
1. Verifies null/zero guards: `engine_send_key_ex(NULL, ...)`, `engine_send_key_ex(engine, 0, ...)` return -1
2. Verifies `!use_xtest` returns -1

These tests don't need an X server because they return early.

Create `tests/test_engine_integration.c` with a test that verifies the structural contract — not actual XTest calls but the input/output contract of each helper:

```c
// Test that keysym_to_keycode falls back correctly (requires keysym_util)
// Test that modifier_keys_for_mask produces correct arrays
// Test that inject_key_sequence handles empty modifier list
```

These can be tested by making the helpers non-static (for testing only) or by including the engine.c in the test with `#define static`.

**Recommended approach:** Compile `tests/test_engine_integration.c` with `-DTEST_ENGINE` and in `engine.c` add:
```c
#ifdef TEST_ENGINE
// Expose internals for testing
int engine_keysym_to_keycode(Display *dpy, KeySym sym, int *mods) { ... }
int engine_modifier_keys(int mods, KeyCode *keys, int *count, KeySym self) { ... }
#endif
```

But this is fragile. Better: **test through the public API only** with known-good patterns:

```c
// Test that engine rejects invalid input
int r = engine_send_key_ex(NULL, XK_a, true, 0);
assert(r == -1);

// Test that engine with no XTest returns error
EngineConfig cfg = { .display = NULL, .use_xtest = false };
Engine *e = engine_create(&cfg);
assert(e != NULL);
r = engine_send_key_ex(e, XK_a, true, 0);
assert(r == -1);
engine_destroy(e);
```

This last test actually opens and closes an X Display (engine_create does XOpenDisplay), so it WILL fail without an X server. Accept this limitation — unit test the helpers, integration test requires X server.

### Verification
- `make test` passes (X-related tests guard with `if (!XOpenDisplay(NULL))`)
- Manual: `DISPLAY=:0 make test` runs the X-dependent tests too

---

## Section 9: Clean up config_save_to_file duplication

### Problem
`config_save_to_file()` has the same field list as `config_load_from_file()` but as fprintf calls. Adding a field requires editing two places.

### Design
Use the X Macro table from Section 6 for both parsing and saving. This eliminates the duplication entirely.

---

## Verification Plan

1. `make test` — 100+ tests pass
2. `make clean && make` — binary compiles without warnings
3. `make install` — installs to ~/.local/bin/0-board
4. Manual: launch 0-board, verify all keys work
5. `git diff --stat` — verify no unintended file changes
