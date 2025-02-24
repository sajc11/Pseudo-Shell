# Makefile for gush
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
TARGET = gush

# Define the source and object directories
SRCDIR = src
OBJDIR = obj

# List all source files and generate object file names
SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# Debug information
$(info Sources found: $(SRCS))
$(info Objects to build: $(OBJS))

.PHONY: all clean test

# Default target
all: $(TARGET)
	@echo "Build complete: $(TARGET)"

# Debug build
debug:
	$(MAKE) DEBUG=1

# Create the object directory
$(OBJDIR):
	@echo "Creating directory: $(OBJDIR)"
	mkdir -p $(OBJDIR)

# Link the final executable
$(TARGET): $(OBJS)
	@echo "Linking: $@ from $^"
	$(CC) $(CFLAGS) -o $@ $^

# Generic rule for object files
$(OBJDIR)/%.o: $(SRCDIR)/%.c $(SRCDIR)/shell.h | $(OBJDIR)
	@echo "Compiling: $< -> $@"
	$(CC) $(CFLAGS) -c $< -o $@

# Target to build the parser test program - run with make test_parser
test_parser: tests/test_parser.c src/parser.c src/utils.c src/shell.h
	@echo "Compiling test_parser..."
	$(CC) $(CFLAGS) -I$(SRCDIR) -o test_parser tests/test_parser.c src/parser.c src/utils.c

clean:
	@echo "Cleaning build artifacts"
	rm -rf $(OBJDIR) $(TARGET) test_parser

test:
	@echo "Running tests..."
	@bash tests/run_tests.sh