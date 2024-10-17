CC = gcc
CFLAGS = -Wall -Wextra -g
LIBS = -lraylib -lvrEmu6502
SRC = main.c monitor.c
OBJ = $(SRC:.c=.o)
TARGET = mapache64emu
BIN_FILE = mapache64.bin

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LIBS)

# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Run the program
run: $(TARGET)
	./$(TARGET) $(BIN_FILE)

# Clean up build artifacts
clean:
	rm -f $(OBJ) $(TARGET)

# Phony targets
.PHONY: all clean run
