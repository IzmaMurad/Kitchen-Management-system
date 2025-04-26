# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -pthread -g -std=c11 -D_GNU_SOURCE
LDFLAGS = -pthread -lrt

# Source files
SRCS = main.c chef.c
OBJS = $(SRCS:.c=.o)
TARGET = chef_program

# Default target
all: $(TARGET)

# Link the program
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

# Compile each source file
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	- rm -f $(OBJS) $(TARGET) *.d

# Debug build (adds debug and sanitizer flags)
debug: CFLAGS += -DDEBUG -g -O0 -fsanitize=thread -fsanitize=address
debug: clean all

# Release build (optimized, no debug info)
release: CFLAGS += -O3 -DNDEBUG -march=native
release: clean all

# Run the program
run: all
	./$(TARGET)

# Install development dependencies (Ubuntu)
install-deps:
	sudo apt-get update
	sudo apt-get install -y build-essential

# Mark these targets as phony
.PHONY: all clean debug release run install-deps

