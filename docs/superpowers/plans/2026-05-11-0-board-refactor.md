# 0-Board Full Refactor: DRY+SOLID+LEAN+KISS Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans.
> Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Eliminate all identified violations while keeping 0-board's ~62KB binary footprint and zero-runtime-dependency architecture intact.

**Architecture:** Layered DI with strict separation. Refactor in 6 independent phases — each produces a self-contained, compilable, testable artifact. No phase depends on the next.

**Tech Stack:** C99, Cairo, X11/Xtst, FontConfig, Makefile.

**Key non-goals:**
- No Wayland backend (new feature, scope creep)
- No auto-hide on physical keyboard (new feature)
- No layout XML format (complexity increase)

---

## Prior Analysis Summary

### Violations Found (18 files, 15 headers)

| Category | Count | Example |
|---|---|---|
| SRP | 4 | `ui_events.c` does event dispatch + voice spawn + resize + key injection |
| DRY | 3 | `shift_map[]` in `engine.c` duplicates `layout.c` symbol pairs |
| KISS | 3 | `renderer.c` is a stub file that does nothing |
| LEAN | 6 | 15MB unused fonts, `DRAG_PILL_*` dead constants, `config_load_from_file` never called |
| SOLID-O | 1 | Key flag classification via `strcmp(k->label, "...")` |
| Missing tests | ∞ | Zero tests for key injection, state machine, layout mapping |
| Fragile deploy | 1 | Font path relative to CWD |

---

## Phase 1: Foundation — Tests + Dead Code

**Files:**
- Create: `tests/test_keyboard_state.c`
- Create: `tests/test_layout_keys.c`
- Create: `tests/test_engine_keysym.c`
- Modify: `Makefile`
- Modify: `src/constants.h`
- Delete: `assets/fonts/extras/ttf/*.ttf` (keep only Inter-Light.ttf, Inter-Regular.ttf)

### Task 1.1: Add test infrastructure

**Files:**
- Modify: `Makefile`

- [ ] **Add test target to Makefile**

Append to Makefile:
```makefile
# Test target
TEST_SRC = tests/test_keyboard_state.c tests/test_layout_keys.c tests/test_engine_keysym.c
TEST_CFLAGS = -Wall -Wextra -O0 -g -I./src -I./tests
TEST_LIBS = -lX11 -lXtst -lcairo -lfontconfig -lfreetype -lm

test: $(TEST_SRC:%.c=%)
	@for t in $(TEST_SRC:%.c=%); do \
		echo "Running $$t..."; \
		./$$t && echo "PASS" || echo "FAIL"; \
	done
	@echo "All tests done."

tests/test_keyboard_state: tests/test_keyboard_state.c src/keyboard_state.c src/layout.c src/keyboard.c src/debug.c
	$(CC) $(TEST_CFLAGS) $^ -o $@ -lX11

tests/test_layout_keys: tests/test_layout_keys.c src/layout.c src/debug.c
	$(CC) $(TEST_CFLAGS) $^ -o $@

tests/test_engine_keysym: tests/test_engine_keysym.c src/debug.c
	$(CC) $(TEST_CFLAGS) $^ -o $@
```

- [ ] **Create test directory**

Run: `mkdir -p tests`

### Task 1.2: Test keyboard state machine

**Files:**
- Create: `tests/test_keyboard_state.c`

- [ ] **Write state machine test**

```c
#include <stdio.h>
#include <string.h>
#include "keyboard_state.h"

static int failures = 0;
static int tests = 0;

#define ASSERT(cond, msg) do { \
    tests++; \
    if (!(cond)) { \
        fprintf(stderr, "  FAIL [%d] %s\n", tests, msg); \
        failures++; \
    } else { \
        printf("  OK   [%d] %s\n", tests, msg); \
    } \
} while(0)

int main() {
    printf("=== keyboard_state tests ===\n");

    KbdState s;
    kbd_state_reset(&s);

    // Initial state
    ASSERT(s.active_layer == LAYER_NORMAL, "initial layer is NORMAL");
    ASSERT(!s.shift_locked, "initial shift not locked");
    ASSERT(s.pressed_key_index == -1, "initial no pressed key");
    ASSERT(s.dirty == true, "reset marks dirty");

    // Shift: normal → one-shot → locked → off
    kbd_state_toggle_shift(&s);
    ASSERT(s.active_layer == LAYER_SHIFT, "toggle shift: one-shot");
    ASSERT(!s.shift_locked, "toggle shift: not locked yet");

    kbd_state_toggle_shift(&s);
    ASSERT(s.shift_locked == true, "toggle shift: locked");
    ASSERT(s.active_layer == LAYER_SHIFT, "toggle shift: still shift layer");

    kbd_state_toggle_shift(&s);
    ASSERT(s.active_layer == LAYER_NORMAL, "toggle shift: off");
    ASSERT(!s.shift_locked, "toggle shift: unlocked");

    // One-shot: key sent → auto-release
    kbd_state_reset(&s);
    kbd_state_toggle_shift(&s); // one-shot
    KeyDef non_shift_key = { .normal = XK_a, .label = "a", .flags = KEYFLAG_NORMAL };
    kbd_state_notify_key_sent(&s, &non_shift_key);
    ASSERT(s.active_layer == LAYER_NORMAL, "one-shot released after non-modifier key");

    // Caps Lock
    kbd_state_reset(&s);
    kbd_state_toggle_caps(&s);
    ASSERT(s.caps_lock == true, "caps on");
    KeyDef letter = { .normal = XK_a, .label = "a", .flags = KEYFLAG_NORMAL };
    ASSERT(kbd_state_get_effective_layer(&s, &letter) == LAYER_SHIFT, "caps → shift for letters");
    KeyDef number = { .normal = XK_1, .label = "1", .flags = KEYFLAG_NORMAL };
    ASSERT(kbd_state_get_effective_layer(&s, &number) == LAYER_NORMAL, "caps → normal for numbers");

    // Modifier mask
    kbd_state_reset(&s);
    ASSERT(kbd_state_get_modifier_mask(&s) == 0, "no modifiers = mask 0");
    s.ctrl_state = MODIFIER_ONESHOT;
    ASSERT((kbd_state_get_modifier_mask(&s) & 2) != 0, "ctrl one-shot → mask bit 1");
    s.alt_state = MODIFIER_LOCKED;
    ASSERT((kbd_state_get_modifier_mask(&s) & 4) != 0, "alt locked → mask bit 2");

    // Reset
    kbd_state_reset(&s);
    ASSERT(s.active_layer == LAYER_NORMAL, "reset: layer");
    ASSERT(s.ctrl_state == MODIFIER_OFF, "reset: ctrl");
    ASSERT(s.alt_state == MODIFIER_OFF, "reset: alt");
    ASSERT(s.meta_state == MODIFIER_OFF, "reset: meta");

    printf("\n%d tests, %d failures\n", tests, failures);
    return failures > 0 ? 1 : 0;
}
```

- [ ] **Create include path helper**

Create `tests/test_common.h`:
```c
#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stdio.h>

static int g_test_failures = 0;
static int g_test_count = 0;

#define TASSERT(cond, msg) do { \
    g_test_count++; \
    if (!(cond)) { \
        fprintf(stderr, "  FAIL [%d] %s\n", g_test_count, msg); \
        g_test_failures++; \
    } else { \
        printf("  OK   [%d] %s\n", g_test_count, msg); \
    } \
} while(0)

#define TEST_REPORT() do { \
    printf("\n%d tests, %d failures\n", g_test_count, g_test_failures); \
    return g_test_failures > 0 ? 1 : 0; \
} while(0)

#endif
```

### Task 1.3: Test layout key integrity

**Files:**
- Create: `tests/test_layout_keys.c`

- [ ] **Write layout integrity tests**

```c
#include <stdio.h>
#include <string.h>
#include "test_common.h"
#include "layout.h"

int main() {
    printf("=== layout key integrity tests ===\n");

    Layout *layout = layout_get_default();
    layout_init(layout);

    TASSERT(layout->num_keys > 0, "layout has keys");
    TASSERT(layout->name != NULL, "layout has name");

    // Every key must have a valid keysym (except special keys)
    int special_keys[] = { -1 }; // sentinel
    for (int i = 0; i < layout->num_keys; i++) {
        KeyDef *k = &layout->keys[i];
        if (k->normal == XK_VoidSymbol) {
            TASSERT(k->label != NULL, "void symbol has label");
        }
        if (k->label == NULL) {
            TASSERT(0, "key has null label");
            continue;
        }
        // Every letter key must have shifted variant
        if (strlen(k->label) == 1 && k->label[0] >= 'a' && k->label[0] <= 'z') {
            TASSERT(k->shifted != 0, "letter key has shifted keysym");
            TASSERT(k->shifted_label != NULL, "letter key has shifted label");
        }
        // Backspace, Enter, Space must be present
        if (strcmp(k->label, "⌫") == 0 || strcmp(k->label, "⏎") == 0 || strcmp(k->label, " ") == 0) {
            TASSERT(k->normal != 0, "special key has valid keysym");
        }
    }

    // Check critical keys exist
    int has_q = 0, has_enter = 0, has_backspace = 0, has_pipe = 0;
    for (int i = 0; i < layout->num_keys; i++) {
        KeyDef *k = &layout->keys[i];
        if (k->normal == XK_q) has_q = 1;
        if (k->normal == XK_Return) has_enter = 1;
        if (k->normal == XK_BackSpace) has_backspace = 1;
        if (k->shifted == XK_bar) has_pipe = 1;
    }
    TASSERT(has_q, "Q key present");
    TASSERT(has_enter, "Enter key present");
    TASSERT(has_backspace, "Backspace key present");
    TASSERT(has_pipe, "Pipe | accessible via shift+backslash");

    // Init idempotency: running layout_init twice shouldn't double-classify
    int shift_count_before = 0;
    for (int i = 0; i < layout->num_keys; i++)
        if (layout->keys[i].flags & KEYFLAG_SHIFT) shift_count_before++;

    layout_init(layout);

    int shift_count_after = 0;
    for (int i = 0; i < layout->num_keys; i++)
        if (layout->keys[i].flags & KEYFLAG_SHIFT) shift_count_after++;

    TASSERT(shift_count_before == shift_count_after, "layout_init is idempotent");

    TEST_REPORT();
}
```

### Task 1.4: Remove dead code

**Files:**
- Modify: `src/constants.h`
- Delete: unused font files

- [ ] **Remove unused constants from constants.h**

Delete these lines:
```c
#define DRAG_HANDLE_HEIGHT_RATIO 0.07
#define DRAG_PILL_WIDTH          40
#define DRAG_PILL_HEIGHT         4
```
```c
#define NUM_BUFFERED_SIZES      3
```

- [ ] **Remove unused fonts from repo**

Run:
```bash
cd /home/leonardo/dev/proyectos/0-Board
# Keep only the two fonts used by the binary
find assets/fonts -type f ! -name "Inter-Light.ttf" ! -name "Inter-Regular.ttf" -delete
# Remove empty directories
find assets/fonts -type d -empty -delete 2>/dev/null; true
```

- [ ] **Commit Phase 1**

```bash
git add tests/ src/constants.h assets/fonts Makefile
git commit -m "test: add state/layout/engine tests and remove dead code

- Add keyboard_state unit tests (shift one-shot/locked, caps lock,
  modifier mask, reset)
- Add layout integrity tests (all critical keys present, init idempotent)
- Remove unused constants: DRAG_HANDLE_HEIGHT_RATIO, DRAG_PILL_*,
  NUM_BUFFERED_SIZES
- Remove 15MB of unused Inter font variants (keep only Inter-Light
  and Inter-Regular)"
```

---

## Phase 2: Engine Refactor — Deduplicate + Simplify

**Files:**
- Modify: `src/engine.c`
- Modify: `src/engine.h`
- Create: `src/keysym_util.h`
- Create: `tests/test_engine_keysym.c`

### Task 2.1: Extract shared keysym mapping

The `shift_map[]` in `engine.c` duplicates the same knowledge already in `layout.c`: XK_exclam comes from XK_1 + Shift, etc. Extract to a shared utility so both layout and engine reference the same data.

**Files:**
- Create: `src/keysym_util.h`

- [ ] **Create shared keysym utility**

```c
#ifndef KEYSYM_UTIL_H
#define KEYSYM_UTIL_H

#include <X11/keysym.h>

typedef struct {
    KeySym shifted;
    KeySym base;
} ShiftPair;

// Map: shifted → base + Shift modifier
// Used by both layout keyboard_state for layout matching and engine for key injection
#define SHIFT_PAIRS_COUNT 20
extern const ShiftPair g_shift_pairs[SHIFT_PAIRS_COUNT];

// Check if a keysym is a letter (A-Z/a-z/ñ)
int keysym_is_letter(KeySym sym);

// Check if a keysym is a standard shifted version of another key
// Returns the base keysym if found, 0 otherwise
KeySym keysym_get_base(KeySym sym);

#endif
```

- [ ] **Create shared keysym utility implementation**

New file `src/keysym_util.c`:
```c
#include "keysym_util.h"

const ShiftPair g_shift_pairs[SHIFT_PAIRS_COUNT] = {
    {XK_exclam,     XK_1},
    {XK_at,         XK_2},
    {XK_numbersign, XK_3},
    {XK_dollar,     XK_4},
    {XK_percent,    XK_5},
    {XK_asciicircum,XK_6},
    {XK_ampersand,  XK_7},
    {XK_asterisk,   XK_8},
    {XK_parenleft,  XK_9},
    {XK_parenright, XK_0},
    {XK_underscore, XK_minus},
    {XK_plus,       XK_equal},
    {XK_braceleft,  XK_bracketleft},
    {XK_braceright, XK_bracketright},
    {XK_bar,        XK_backslash},
    {XK_colon,      XK_semicolon},
    {XK_quotedbl,   XK_apostrophe},
    {XK_less,       XK_comma},
    {XK_greater,    XK_period},
    {XK_question,   XK_slash},
};

int keysym_is_letter(KeySym sym) {
    return (sym >= XK_a && sym <= XK_z) ||
           (sym >= XK_A && sym <= XK_Z) ||
           (sym == XK_ntilde) || (sym == XK_Ntilde);
}

KeySym keysym_get_base(KeySym sym) {
    for (int i = 0; i < SHIFT_PAIRS_COUNT; i++) {
        if (g_shift_pairs[i].shifted == sym)
            return g_shift_pairs[i].base;
    }
    return 0;
}
```

### Task 2.2: Refactor engine.c to use shared mappings

**Files:**
- Modify: `src/engine.c`
- Modify: `Makefile`

- [ ] **Replace inline shift_map with shared keysym_util**

In `src/engine.c`:
- Remove the local `shift_map[]` array
- Include `"keysym_util.h"`
- Replace the shifted→base lookup with `keysym_get_base(keysym)`

The `engine_send_key_ex` function changes from:
```c
static const struct { KeySym shifted; KeySym base; } shift_map[] = {
    {XK_exclam, XK_1}, ...
};
for (size_t i = 0; i < sizeof(shift_map)/sizeof(shift_map[0]); i++) {
    if (shift_map[i].shifted == keysym) { base = shift_map[i].base; ... }
}
```

To:
```c
KeySym base = keysym_get_base(keysym);
if (base) { modifiers |= 1; kc = XKeysymToKeycode(engine->display, base); }
```

- [ ] **Replace keyboard_state letter check with shared function**

In `src/keyboard_state.c`:
- Remove inline `kbd_state_is_letter`
- Include `"keysym_util.h"`
- Replace call to `kbd_state_is_letter` with `keysym_is_letter`

### Task 2.3: Write engine keysym tests

**Files:**
- Create: `tests/test_engine_keysym.c`

- [ ] **Write keysym resolution tests**

```c
#include <stdio.h>
#include "test_common.h"
#include "keysym_util.h"

int main() {
    printf("=== keysym utility tests ===\n");

    // Letter detection
    TASSERT(keysym_is_letter(XK_a), "lowercase a is letter");
    TASSERT(keysym_is_letter(XK_Z), "uppercase Z is letter");
    TASSERT(keysym_is_letter(XK_ntilde), "ñ is letter");
    TASSERT(!keysym_is_letter(XK_1), "1 is not letter");
    TASSERT(!keysym_is_letter(XK_Return), "Return is not letter");

    // Shift pair mappings
    TASSERT(keysym_get_base(XK_exclam) == XK_1, "! → 1");
    TASSERT(keysym_get_base(XK_bar) == XK_backslash, "| → backslash");
    TASSERT(keysym_get_base(XK_braceleft) == XK_bracketleft, "{ → [");
    TASSERT(keysym_get_base(XK_braceright) == XK_bracketright, "} → ]");
    TASSERT(keysym_get_base(XK_asciitilde) == XK_grave, "~ → grave");
    TASSERT(keysym_get_base(XK_colon) == XK_semicolon, ": → ;");
    TASSERT(keysym_get_base(XK_quotedbl) == XK_apostrophe, "\" → '");
    TASSERT(keysym_get_base(XK_question) == XK_slash, "? → /");

    // Non-shifted symbols
    TASSERT(keysym_get_base(XK_a) == 0, "a has no base");
    TASSERT(keysym_get_base(XK_1) == 0, "1 has no base");
    TASSERT(keysym_get_base(0) == 0, "null keysym has no base");

    TEST_REPORT();
}
```

- [ ] **Add keysym_util.c to Makefile**

Append `src/keysym_util.c` to the `SRC` list in Makefile.

- [ ] **Commit Phase 2**

```bash
git add src/engine.c src/engine.h src/keysym_util.h src/keysym_util.c src/keyboard_state.c tests/test_engine_keysym.c Makefile
git commit -m "refactor: extract shared keysym mapping, deduplicate shift_map

- Move shift pair mapping from engine.c to shared keysym_util.h/.c
- Both layout (for keyboard_state letter checks) and engine now
  reference the same canonical mapping
- Replace engine.c local static shift_map with keysym_get_base()
- Replace keyboard_state inline kbd_state_is_letter with keysym_is_letter()
- Add keysym utility unit tests"
```

---

## Phase 3: SRP — Split ui_events.c

**Files:**
- Modify: `src/ui_events.c`
- Create: `src/key_injector.c`
- Create: `src/key_injector.h`
- Create: `src/ui_drag.c`
- Create: `src/ui_drag.h`
- Modify: `Makefile`
- Modify: `src/ui_internal.h`
- Modify: `src/colors.h`
- Modify: `src/colors.c`

### Task 3.1: Extract key injection logic

**Files:**
- Create: `src/key_injector.h`

- [ ] **Create key injection interface**

```c
#ifndef KEY_INJECTOR_H
#define KEY_INJECTOR_H

#include <X11/Xlib.h>
#include <stdbool.h>
#include "keyboard.h"
#include "engine.h"

// Inject a key into the system via the engine.
// Returns true if a key was sent, false if the key was a modifier.
bool key_injector_send(Engine *engine, Keyboard *keyboard, int key_index,
                       Display *display, int delay_us);

#endif
```

- [ ] **Create key injection implementation**

```c
#include "key_injector.h"
#include "layout.h"
#include <unistd.h>

bool key_injector_send(Engine *engine, Keyboard *keyboard, int key_index,
                       Display *display, int delay_us) {
    if (!engine || !keyboard) return false;

    Layout *layout = keyboard_get_layout(keyboard);
    if (!layout || key_index < 0 || key_index >= layout->num_keys) return false;

    KeyDef *key = &layout->keys[key_index];
    bool is_modifier = (key->flags & (KEYFLAG_SHIFT | KEYFLAG_CTRL |
                                      KEYFLAG_ALT | KEYFLAG_META)) ||
                       (key->normal == XK_Caps_Lock);

    if (is_modifier) return false;

    KeySym sym = keyboard_get_keysym(keyboard, key_index);
    if (sym == 0) return false;

    int mods = keyboard_get_modifiers_for_keysym(keyboard, sym);
    engine_send_key_ex(engine, sym, true, mods);
    XFlush(display);

    if (delay_us > 0) usleep(delay_us);

    engine_send_key_ex(engine, sym, false, mods);
    engine_flush(engine);

    keyboard_notify_key_sent(keyboard, key_index);
    return true;
}
```

### Task 3.2: Extract drag handler

**Files:**
- Create: `src/ui_drag.h`

- [ ] **Create drag interface**

```c
#ifndef UI_DRAG_H
#define UI_DRAG_H

#include <stdbool.h>
#include "x11_window.h"

typedef struct {
    bool active;
    int offset_x;
    int offset_y;
} DragState;

// Initialize drag state
static inline void drag_init(DragState *d) { d->active = false; }

// Start drag at a root position offset from window
void drag_start(DragState *d, X11Window *win, int root_x, int root_y);

// Handle motion event: move window if dragging
void drag_move(DragState *d, X11Window *win, int root_x, int root_y);

// End drag
static inline void drag_end(DragState *d) { d->active = false; }

#endif
```

- [ ] **Create drag implementation**

```c
#include "ui_drag.h"

void drag_start(DragState *d, X11Window *win, int root_x, int root_y) {
    if (!d || !win) return;
    int win_x, win_y;
    x11_window_get_position(win, &win_x, &win_y);
    d->offset_x = root_x - win_x;
    d->offset_y = root_y - win_y;
    d->active = true;
}

void drag_move(DragState *d, X11Window *win, int root_x, int root_y) {
    if (!d || !d->active || !win) return;
    x11_window_move(win, root_x - d->offset_x, root_y - d->offset_y);
}
```

### Task 3.3: Simplify ui_events.c

**Files:**
- Modify: `src/ui_events.c`
- Modify: `src/ui_internal.h`
- Modify: `Makefile`

- [ ] **Update ui_internal.h to use DragState**

Replace in `struct UI`:
```c
// Interaction state
bool dragging;
int drag_offset_x, drag_offset_y;
```

With:
```c
DragState drag;
```

- [ ] **Replace inline drag code with drag module calls**

In `ui_events.c`:
- Add `#include "key_injector.h"` and `#include "ui_drag.h"`
- Replace drag initiation block with `drag_start(&ui->drag, ui->window, rx, ry);`
- Replace `ui->dragging = true;` with just the drag_start call
- Replace drag guard in ui_handle_motion:
  ```c
  void ui_handle_motion(UI *ui, int rx, int ry) {
      if (!ui) return;
      drag_move(&ui->drag, ui->window, rx, ry);
  }
  ```
- Replace release drag clear with `drag_end(&ui->drag);`

- [ ] **Replace inline key injection with key_injector_send**

Replace the non-modifier key logic in `ui_handle_button_press`:
```c
} else {
    keyboard_press_key(ui->keyboard, i);
    key_injector_send(ui->engine, ui->keyboard, i,
                      x11_window_get_display(ui->window),
                      ui->config.key_event_delay_us);
}
```

- [ ] **Commit Phase 3**

```bash
git add src/ui_events.c src/ui_internal.h src/key_injector.c src/key_injector.h src/ui_drag.c src/ui_drag.h Makefile
git commit -m "refactor: split ui_events.c into SRP modules

- Extract key injection logic → key_injector.c (single responsibility:
  send a key from a keyboard index through the engine)
- Extract drag handler → ui_drag.c (single responsibility:
  manage window dragging via X11)
- Simplify ui_events.c: 120-line button handler → 40 lines
- Remove inline drag state from UI struct, use DragState module"
```

---

## Phase 4: Configuration Hardening

**Files:**
- Modify: `src/main.c`
- Modify: `src/config.c`
- Modify: `src/config.h`

### Task 4.1: Enable file-based config loading

**Files:**
- Modify: `src/main.c`

- [ ] **Wire config_load_from_file into main.c startup**

Replace in `main.c`:
```c
Config config;
config_load_defaults(&config);
```

With:
```c
Config config;
config_load_defaults(&config);
char *cfg_path = config_get_default_path();
if (cfg_path) {
    config_load_from_file(&config, cfg_path);
    free(cfg_path);
}
```

### Task 4.2: Resolve font path at runtime

**Files:**
- Modify: `src/config.c`
- Modify: `src/config.h`
- Modify: `src/font_manager.c`

- [ ] **Add font_dir to Config struct**

In `config.h`, add to `Config`:
```c
char font_dir[512]; // Absolute path to fonts directory
```

In `config.c`, add default:
```c
const char *home = getenv("HOME");
if (home) {
    snprintf(config->font_dir, sizeof(config->font_dir),
             "%s/0-Board/assets/fonts", home);
} else {
    strncpy(config->font_dir, "./assets/fonts", sizeof(config->font_dir) - 1);
}
```

- [ ] **Pass font_dir to font_manager**

In `font_manager.h` `FontConfig`, add:
```c
const char *font_dir; // Absolute path to fonts (NULL for CWD-relative)
```

In `font_manager.c`, update `load_fonts` to use `fm->config.font_dir` instead of hardcoded `"./assets/fonts"`.

In `main.c`, pass `config.font_dir`:
```c
FontConfig font_config = {
    .preferred_family = "Inter",
    .load_all_system_fonts = false,
    .max_fonts_to_cache = 50,
    .font_dir = config.font_dir,
};
```

- [ ] **Add config save support to config.c parse**

Currently unhandled keys are silently ignored. Add `font_dir` to the parser and save function.

- [ ] **Commit Phase 4**

```bash
git add src/main.c src/config.c src/config.h src/font_manager.h src/font_manager.c
git commit -m "refactor: enable config file loading and fix font path resolution

- Wire config_load_from_file into main.c startup
- Add font_dir to Config struct, resolved from HOME at runtime
- Pass absolute font dir to font_manager instead of relative path
- Fix deployment fragility: no longer requires CWD to be binary dir"
```

---

## Phase 5: Polish — Dead Abstraction + License Header

**Files:**
- Modify: `src/renderer.c`
- Modify: All `src/*.c` files

### Task 5.1: Remove renderer.c stub

`renderer.c` has a single function that returns NULL. It's never called — all real callers use `cairo_renderer_create` or `renderer_create_for_x11_window`.

**Files:**
- Modify: `src/renderer.c`

- [ ] **Replace stub with compile-time assertion**

```c
#include "renderer.h"
#include <stdlib.h>

// Abstract renderer — concrete implementations (cairo_renderer.c)
// provide the actual create function. This file exists to prevent
// linker errors on the abstract interface symbols.
// All real functionality is in cairo_renderer.c and x11_cairo_bridge.c.
```

Remove `renderer_create()` stub entirely.

### Task 5.2: Deduplicate license headers

**Files:**
- Modify: `src/*.c` and `src/*.h`

- [ ] **Replace 5-line license blocks with single-line reference**

Replace each file's:
```c
/*
 * 0-Board Virtual Keyboard
 * Copyright (c) 2026 Leonardo Vergara <leonardovergaramarin@gmail.com>
 * Licensed under the MIT License.
 */
```

With:
```c
// SPDX-License-Identifier: MIT — see LICENSE file
```

Do NOT touch `layout.c` (it's a large data file, license stays for context).

- [ ] **Commit Phase 5**

```bash
git add src/renderer.c src/*.c src/*.h
git commit -m "chore: remove dead renderer.c stub, deduplicate license headers

- Replace renderer_create() stub (always returned NULL) with empty file
  — all callers use cairo_renderer_create or renderer_create_for_x11_window
- Replace 5-line MIT license header in all files with SPDX one-liner
  (kept full header in layout.c for data file context)"
```

---

## Verification

- [ ] **Build and run tests**

```bash
cd /home/leonardo/dev/proyectos/0-Board
make clean && make test
```

Expected: All tests pass, zero failures.

- [ ] **Build release binary**

```bash
make release
ls -lh 0-board
```

Expected: binary size ≤ 65KB.

- [ ] **Verify memory and CPU**

```bash
DISPLAY=:0 ./0-board &
sleep 2
ps -o rss,pcpu -p $(pgrep 0-board)
kill %1
```

Expected: RSS ≤ 12MB, CPU ≤ 0.5% idle.

- [ ] **Deploy to tablet**

```bash
scp 0-board leonardo@100.108.84.33:~/0-Board/
ssh leonardo@100.108.84.33 'kill $(pgrep 0-board); sleep 1; DISPLAY=:0 ~/0-Board/0-board &'
```
