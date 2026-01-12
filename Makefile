# Variables
CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = cantest
SRC = cantest.c

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) -Wno-unused-parameter

# Clean up build files
clean:
	rm -f $(TARGET)

	