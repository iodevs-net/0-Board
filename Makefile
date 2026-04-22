CC = gcc
CFLAGS = -Wall -Wextra -O3 -flto -march=native -I./src \
         $(shell pkg-config --cflags cairo fontconfig freetype2)
LIBS = -lX11 -lXtst -lcairo -lfontconfig -lfreetype -lm

# Source files
SRC = src/layout.c src/keyboard.c src/keyboard_state.c src/colors.c src/config.c \
      src/renderer.c src/cairo_renderer.c \
      src/engine.c src/x11_window.c src/font_manager.c src/x11_cairo_bridge.c \
      src/ui.c src/ui_events.c src/ui_render_helper.c src/layout_engine.c \
      src/main.c src/debug.c

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

.PHONY: all debug release clean