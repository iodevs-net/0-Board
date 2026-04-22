# 0-board

A modern virtual keyboard built from scratch with a focus on clean architecture, maintainability, and IA-agent friendliness.

## Philosophy

- **Pure C Architecture**: Escrito en C99 puro para máxima eficiencia, portabilidad y un consumo de recursos cercano a cero. Sin dependencias de C++ ni runtimes pesados.
- **Clean Architecture**: Dependency injection, no global state, SRP (modules ≤100 lines).
- **Maintainability**: Self-documenting code, constants in `constants.h`, error handling via `error_exit()`.
- **IA-agent Readability**: Predictable structure, minimal magic numbers, clear interfaces.
- **LEAN**: Lazy loading of fonts (~5MB RAM), double buffer optimized, event-driven rendering.

## Why "0-Board"? (Zero-Resource Architecture)

0-Board is designed to achieve near-zero CPU and memory footprint:
- **Surface Caching**: Static elements are pre-rendered once. Typing costs only a simple `BitBlt` operation.
- **Event-Driven**: Consumes 0% CPU while idle by blocking on system events (no polling).
- **Metadata Pre-calculation**: All font scales and layout math are pre-calculated to avoid logic branches in the render loop.
- **Minimal Footprint**: ~10MB RSS RAM usage. No heavy frameworks (Qt/GTK) or Electron bloat.

### Benchmark Comparison

| Metric | xvkbd (The Classic) | Electron-based | **0-Board** |
| :--- | :--- | :--- | :--- |
| **Idle CPU** | < 0.1% | 5-10% | **< 0.1%** |
| **Memory (RSS)** | ~7MB | 150MB+ | **~10MB** |
| **Startup Time** | ~0.1s | ~3.0s | **< 0.1s** |
| **Graphics** | Bitmap/Primitive | Browser Engine | **Vector (Cairo)** |
| **Binary Size** | ~150KB | 100MB+ | **~180KB** |

## User Experience (UX)

0-Board is designed to be as comfortable for the user as it is efficient for the system:
- **Easy Resizing**: Cycle through Small, Medium, and Large sizes with a single tap.
- **Custom Transparency**: Real-time opacity control to balance visibility and screen real estate.
- **Modern Themes**: Support for curated Light and Dark modes.
- **Compact Format**: Minimalist design that maximizes screen space on tablets and small screens.
- **Quick Docking**: Instantly toggle between top and bottom screen positions.
- **Audit-Ready**: Código documentado profundamente para ser fácil de auditar, mantener y personalizar.

## Key Features

- **Vector Rendering**: Cairo-based, antialiased, native transparency.
- **Minimalist UI**: Focus on functionality, real-time customization (opacity, color schemes, fonts).
- **Backend Abstraction**: X11 backend with Wayland-ready interfaces.
- **Modular Design**: Separated into core (keyboard, layout), render (graphics abstraction), platform (system-specific), and UI coordination.

## Project Structure

```
0-board/
├── src/
│   ├── core/         # Business logic (keyboard, layout)
│   ├── render/       # Graphics abstraction (Cairo implementation)
│   ├── platform/     # System-specific backends (X11)
│   └── ui/           # UI coordination
├── assets/           # Fonts and other assets
├── bench/            # Benchmarks
└── Makefile
```

## Build and Run

### Dependencies

- Debian/Ubuntu: `sudo apt install libcairo2-dev libx11-dev libxtst-dev libfontconfig1-dev pkg-config`
- Fedora/RHEL: `sudo dnf install cairo-devel libX11-devel libXtst-devel pkg-config`

### Compilation

```bash
make          # produces ./0-board
make debug    # debug build with logs
make release  # release build
make clean
```

### Execution

```bash
./0-board
```

## Code Conventions

- Constants defined in `constants.h` (no magic numbers).
- Self-documenting names preferred over comments.
- Error handling via `error_exit()` function.
- Debug logging with `DEBUG_INIT()` and `LOG_*` macros (enabled in debug build).
- Testability through dependency injection (no complex mocks needed).

## Entry Point

Execution begins in `src/main.c` and follows the sequence:
   config → layout → keyboard → font manager → window → renderer → engine → UI → main loop

## Verification

- No formal test suite; verification via manual execution and debug logs.
- Debug logs written to `debug.log` when built with `make debug`.

## Contributing

This project is designed for contribution by both humans and IA agents. Please adhere to:
   1. SRP: Keep modules ≤100 lines.
   2. Use constants.h for all magic numbers.
   3. Prefer self-documenting code over comments.
   4. Dependencies injected, not global.
   5. Interfaces abstract for backend interchangeability.

## License

MIT - See LICENSE file for details.