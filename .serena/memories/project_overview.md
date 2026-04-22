# Project Purpose and Tech Stack

## Purpose
A premium virtual keyboard for Linux (X11) for 10-inch tablets, designed as an Apple Magic Keyboard for programmers. Follows DRY, SOLID, KISS, LEAN principles.

## Tech Stack
- **Languages**: C (GCC)
- **Graphics**: Cairo (vector graphics, antialiasing)
- **Windowing/Events**: X11 (libX11, libXtst for input simulation)
- **Fonts**: FontConfig + FreeType (bundled Inter font in assets/fonts/)
- **Build System**: GNU Make

## Key Architecture
- `layout.c` — 5-row Magic Keyboard layout with all programmer keys
- `colors.c` — RGBA color system: Space Gray + Silver themes
- `ui_render_helper.c` — Premium rendering (key shadows, accent borders, per-type font scaling)
- `ui_events.c` — Touch-friendly event handling with proportional drag zones
- Auto-positions at bottom-center of screen
