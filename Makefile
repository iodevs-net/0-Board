PREFIX ?= $(HOME)/.local
BINDIR ?= $(PREFIX)/bin
DATADIR ?= $(PREFIX)/share/0-board

CC = gcc
CFLAGS = -Wall -Wextra -O3 -flto -march=native -I./src \
         $(shell pkg-config --cflags cairo fontconfig freetype2)
LIBS = -lX11 -lXtst -lcairo -lfontconfig -lfreetype -lm

# Source files
SRC = src/layout.c src/keyboard.c src/keyboard_state.c src/colors.c src/config.c \
      src/cairo_renderer.c \
      src/engine.c src/x11_window.c src/x11_events.c src/font_manager.c src/x11_cairo_bridge.c \
      src/ui.c src/ui_events.c src/ui_render_helper.c src/layout_engine.c \
      src/main.c src/debug.c \
    src/keysym_util.c src/keysym_layout.c \
      src/key_injector.c src/ui_drag.c src/touchpad.c

OBJ = $(SRC:.c=.o)
TARGET = 0-board
HELPERS = 0-board-touch-handler 0-board-lock-monitor

all: $(TARGET) $(HELPERS)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET) $(LIBS)

0-board-touch-handler: src/0-board-touch-handler.c
	$(CC) $(CFLAGS) $< -o $@ -lX11 -lXtst -lXi

0-board-lock-monitor: src/0-board-lock-monitor.c
	$(CC) $(CFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Development: debug symbols + logs
debug: CFLAGS += -g -DDEBUG -O0
debug: clean $(TARGET) $(HELPERS)

# Production: max optimization, no logs
release: CFLAGS += -DNDEBUG
release: clean $(TARGET) $(HELPERS)

clean:
	rm -f src/*.o $(TARGET) $(HELPERS)

install: release
	install -d $(BINDIR) $(DATADIR)/fonts
	install -m 755 0-board $(BINDIR)/
	install -m 755 0-board-touch-handler $(BINDIR)/
	install -m 755 0-board-lock-monitor $(BINDIR)/
	install -m 644 assets/fonts/extras/ttf/Inter-Light.ttf $(DATADIR)/fonts/
	install -m 644 assets/fonts/extras/ttf/Inter-Regular.ttf $(DATADIR)/fonts/
	@echo "0-board and native helpers installed to $(BINDIR)/"
	@echo "Fonts installed to $(DATADIR)/fonts/"
	@echo "Set font_dir = $(DATADIR)/fonts in ~/.config/0-board/config.ini for portable install"


.PHONY: all debug release clean test install

# Test target
TEST_SRC = tests/test_keyboard_state.c tests/test_layout_keys.c tests/test_engine_keysym.c tests/test_engine_integration.c
TEST_CFLAGS = -Wall -Wextra -O0 -g -I./src -I./tests
TEST_LIBS = -lX11 -lXtst -lcairo -lfontconfig -lfreetype -lm

test: $(TEST_SRC:%.c=%)
	@for t in $(TEST_SRC:%.c=%); do \
		echo "Running $$t..."; \
		./$$t && echo "PASS" || echo "FAIL"; \
	done
	@echo "All tests done."

tests/test_keyboard_state: tests/test_keyboard_state.c src/keyboard_state.c src/layout.c src/keyboard.c src/debug.c src/keysym_util.c
	$(CC) $(TEST_CFLAGS) $^ -o $@ -lX11

tests/test_layout_keys: tests/test_layout_keys.c src/layout.c src/debug.c
	$(CC) $(TEST_CFLAGS) $^ -o $@

tests/test_engine_keysym: tests/test_engine_keysym.c src/keysym_util.c
	$(CC) $(TEST_CFLAGS) $^ -o $@
tests/test_engine_integration: tests/test_engine_integration.c src/engine.c src/keysym_util.c src/keysym_layout.c src/debug.c
	$(CC) $(TEST_CFLAGS) $^ -o $@ -lX11 -lXtst