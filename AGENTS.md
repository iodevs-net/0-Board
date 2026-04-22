# 0-board Agent Guidelines

## Build & Run
- Compile: `make` (produces `./0-board`)
- Debug build: `make debug`
- Release build: `make release`
- Clean: `make clean`
- Run: `./0-board`

## Dependencies
- Required: libcairo2-dev, libx11-dev, libxtst-dev, libfontconfig1-dev, pkg-config
- Install (Debian/Ubuntu): `sudo apt install libcairo2-dev libx11-dev libxtst-dev libfontconfig1-dev pkg-config`

## Architecture Notes
- Clean architecture with dependency injection
- No global state (predictability for testing)
- Modules limited to ≤100 lines (SRP)
- Interfaces abstract for backend interchangeability (X11, Wayland-ready)
- Lazy loading of fonts (~5MB RAM usage)

## Key Directories
- `src/core/` - Business logic (keyboard, layout)
- `src/render/` - Graphics abstraction (Cairo implementation)
- `src/platform/` - System-specific backends (X11)
- `src/ui/` - UI coordination

## Code Conventions
- Constants in `constants.h` (no magic numbers)
- Self-documenting names over comments
- Error handling via `error_exit()` function
- Debug logging with `DEBUG_INIT()` and `LOG_*` macros
- Testability through dependency injection (no complex mocks needed)

## Entry Point
- Execution starts in `src/main.c`
- Follows initialization sequence: config → layout → keyboard → font manager → window → renderer → engine → UI → main loop

## Verification
- No formal test suite found; verification through manual execution and debug logs
- Debug logs written to `debug.log` when built with `make debug`