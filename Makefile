CC = gcc
CFLAGS = -Wall -Wextra -O3 -flto -march=native -I./src \
         $(shell pkg-config --cflags cairo fontconfig freetype2)
LIBS = -lX11 -lXtst -lcairo -lfontconfig -lfreetype -lm

# Source files
SRC = src/layout.c src/keyboard.c src/keyboard_state.c src/colors.c src/config.c \
      src/renderer.c src/cairo_renderer.c \
      src/engine.c src/x11_window.c src/font_manager.c src/x11_cairo_bridge.c \
      src/ui.c src/ui_events.c src/ui_render_helper.c src/layout_engine.c \
      src/main.c src/debug.c \
      src/keysym_util.c \
      src/key_injector.c src/ui_drag.c

OBJ = $(SRC:.c=.o)
TARGET = 0-board

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Development: debug symbols + logs
debug: CFLAGS += -g -DDEBUG -O0
debug: clean $(TARGET)

# Production: max optimization, no logs
release: CFLAGS += -DNDEBUG
release: clean $(TARGET)

clean:
	rm -f src/*.o $(TARGET)

.PHONY: all debug release clean test

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
tests/test_engine_integration: tests/test_engine_integration.c src/engine.c src/keysym_util.c src/debug.c
	$(CC) $(TEST_CFLAGS) $^ -o $@ -lX11 -lXtst