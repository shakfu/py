# Makefile for Production Function Cache System

# Detect Python version and paths
PYTHON_VERSION := $(shell python3-config --version 2>/dev/null || echo "3.9")
PYTHON_INCLUDE := $(shell python3-config --includes)
PYTHON_LIBS := $(shell python3-config --ldflags) -lpython3.13

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 -pthread $(PYTHON_INCLUDE)
LDFLAGS = $(PYTHON_LIBS) -lpthread

# Project files
DEMO_TARGET = function_cache_demo
DEMO_SOURCE = function_cache.c
HEADER = function_cache.h

# Library settings
LIB_TARGET = libfunctioncache.so
LIB_CFLAGS = $(CFLAGS) -fPIC -shared

# Default target
all: $(DEMO_TARGET) $(LIB_TARGET)

# Build the demo executable
$(DEMO_TARGET): $(DEMO_SOURCE) $(HEADER)
	$(CC) $(CFLAGS) -o $(DEMO_TARGET) $(DEMO_SOURCE) $(LDFLAGS)

# Build shared library
$(LIB_TARGET): $(DEMO_SOURCE) $(HEADER)
	$(CC) $(LIB_CFLAGS) -o $(LIB_TARGET) $(DEMO_SOURCE) $(LDFLAGS)

# Run the demo
run: $(DEMO_TARGET)
	./$(DEMO_TARGET)

# Run with performance profiling
profile: $(DEMO_TARGET)
	time ./$(DEMO_TARGET)

# Run with memory checking (requires valgrind)
memcheck: $(DEMO_TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(DEMO_TARGET)

# Debug build
debug: CFLAGS += -g -DDEBUG -O0
debug: $(DEMO_TARGET)

# Release build with optimizations
release: CFLAGS += -O3 -DNDEBUG -march=native
release: $(DEMO_TARGET)

# Clean build artifacts
clean:
	rm -f $(DEMO_TARGET) $(LIB_TARGET) *.o *.so

# Show Python configuration (for debugging)
info:
	@echo "Python includes: $(PYTHON_INCLUDE)"
	@echo "Python libs: $(PYTHON_LIBS)"
	@echo "Python version: $(PYTHON_VERSION)"
	@echo "Compiler flags: $(CFLAGS)"
	@echo "Linker flags: $(LDFLAGS)"

# Install dependencies (Ubuntu/Debian)
install-deps:
	sudo apt-get update
	sudo apt-get install python3-dev gcc make valgrind

# Install dependencies (macOS with Homebrew)
install-deps-mac:
	brew install python3 gcc make

# Install dependencies (CentOS/RHEL/Fedora)
install-deps-rpm:
	sudo yum install python3-devel gcc make valgrind

# Format code (requires clang-format)
format:
	clang-format -i $(DEMO_SOURCE) $(HEADER)

# Static analysis (requires cppcheck)
analyze:
	cppcheck --enable=all --std=c99 $(DEMO_SOURCE)

# Create package
package: clean all
	tar -czf function_cache_system.tar.gz $(DEMO_SOURCE) $(HEADER) Makefile README.md

.PHONY: all run profile memcheck debug release clean info install-deps install-deps-mac install-deps-rpm format analyze package

