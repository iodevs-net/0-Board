# 0-board

![0-board Apple-style Virtual Keyboard](assets/images/screenshot.png)

A modern virtual keyboard built from scratch with a focus on clean architecture, maintainability, and IA-agent friendliness.

## Philosophy

- **Pure C Architecture**: Written in pure C99 for maximum efficiency, portability, and near-zero resource consumption. No C++ dependencies or heavy runtimes.
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

| Metric           | xvkbd (The Classic) | Electron-based | **0-Board**        |
| :--------------- | :------------------ | :------------- | :----------------- |
| **Idle CPU**     | < 0.1%              | 5-10%          | **< 0.1%**         |
| **Memory (RSS)** | ~7MB                | 150MB+         | **~10MB**          |
| **Startup Time** | ~0.1s               | ~3.0s          | **< 0.1s**         |
| **Graphics**     | Bitmap/Primitive    | Browser Engine | **Vector (Cairo)** |
| **Binary Size**  | ~150KB              | 100MB+         | **~180KB**         |

## User Experience (UX)

0-Board is designed to be as comfortable for the user as it is efficient for the system:

- **Easy Resizing**: Cycle through Small, Medium, and Large sizes with a single tap.
- **Custom Transparency**: Real-time opacity control to balance visibility and screen real estate.
- **Deep Customization**: Edit `.theme` files in `assets/themes/` to control every color (background, keys, text, etc.).
- **Modern Themes**: Support for curated Light, Dark, and Custom modes.
- **Voice Dictation (Zero-Friction)**: A seamlessly integrated voice-to-text dictation feature triggered by the `mic` key. Uses Whisper-Large-v3 via API. Built with a pure "zero-resource" architecture—instead of heavy IPCs, it relies on a lightweight atomic lock file (`/tmp/0-voice-recording`) that the UI loop polls every 500ms at `0.0%` CPU cost, dynamically overriding the rendering pass to turn the microphone key **red** while listening.
- **Compact Format**: Minimalist design that maximizes screen space on tablets and small screens.
- **Quick Docking**: Instantly toggle between top and bottom screen positions.
- **Audit-Ready**: Deeply documented code for easy auditing, maintenance, and customization.

## Customization

0-Board loves Linux-style customization. You can define your own themes in `assets/themes/`:

```ini
# assets/themes/my_cool_theme.theme
background      = #1c1c1e
text            = #ffffff
key_normal      = #3a3a3c
key_pressed     = #636366
key_modifier    = #2c2c2e  # Shift, Ctrl, Alt
key_text        = #3a3a3c  # Letters
key_number      = #3a3a3c  # Numbers row
key_special     = #48484a  # Enter, Space, Arrows
shift_active    = #0a84ff  # Highlight when Shift is on
```

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

1.  SRP: Keep modules ≤100 lines.
2.  Use constants.h for all magic numbers.
3.  Prefer self-documenting code over comments.
4.  Dependencies injected, not global.
5.  Interfaces abstract for backend interchangeability.

## License

MIT - See LICENSE file for details.
Developed by **Leonardo Vergara** (<leonardovergaramarin@gmail.com>)
