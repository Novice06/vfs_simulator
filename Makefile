FAT = mkfs.fat
CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS =
TARGET = vfs_simulator
SOURCES = $(wildcard *.c)
OBJECTS = $(patsubst %.c, obj/%.o, $(SOURCES))

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

obj/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	./$(TARGET)