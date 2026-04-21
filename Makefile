CC = gcc
CFLAGS = -Wall -Wextra -O2 -I./src $(shell pkg-config --cflags cairo)
LIBS = -lX11 -lXtst $(shell pkg-config --libs cairo)

SRC = src/engine.c src/layout.c src/ui.c src/main.c
OBJ = $(SRC:.c=.o)
TARGET = xvkbd_new

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LIBS)

clean:
	rm -f src/*.o $(TARGET)

.PHONY: all clean
