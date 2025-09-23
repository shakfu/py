# Makefile for Smart Function Caching System

# Python configuration
PYTHON := python3
PYTHON_CONFIG := $(PYTHON)-config
PYTHON_INCLUDE := $(shell $(PYTHON_CONFIG) --includes 2>/dev/null || echo "-I/usr/include/python3")
PYTHON_LIBS := $(shell $(PYTHON_CONFIG) --ldflags 2>/dev/null || echo "-lpython3")

# Compiler settings
CC := gcc
CFLAGS := -Wall -Wextra -O2 -std=c99 $(PYTHON_INCLUDE)
LDFLAGS := $(PYTHON_LIBS) -lpthread

# Source files
DEMO_SOURCE := smart_caching_demo.c
HEADERS := py_function_cache.h smart_caching_heuristics.h

# Targets
DEMO_TARGET := smart_caching_demo

# Default target
all: $(DEMO_TARGET)

# Build the demo
$(DEMO_TARGET): $(DEMO_SOURCE) $(HEADERS)
	$(CC) $(CFLAGS) -o $(DEMO_TARGET) $(DEMO_SOURCE) $(LDFLAGS) -lpython3.13

# Run the demo
run: $(DEMO_TARGET)
	./$(DEMO_TARGET)

# Clean build artifacts
clean:
	rm -f $(DEMO_TARGET) *.o

# Show configuration
info:
	@echo "Python includes: $(PYTHON_INCLUDE)"
	@echo "Python libs: $(PYTHON_LIBS)" 
	@echo "Compiler flags: $(CFLAGS)"
	@echo "Linker flags: $(LDFLAGS)"

# Install dependencies
install-deps:
	@echo "Installing Python development headers..."
	@if command -v apt-get >/dev/null 2>&1; then \
		sudo apt-get update && sudo apt-get install -y python3-dev gcc; \
	elif command -v yum >/dev/null 2>&1; then \
		sudo yum install -y python3-devel gcc; \
	elif command -v brew >/dev/null 2>&1; then \
		brew install python3 gcc; \
	else \
		echo "Please install python3-dev/python3-devel and gcc manually"; \
	fi

.PHONY: all run clean info install-deps

