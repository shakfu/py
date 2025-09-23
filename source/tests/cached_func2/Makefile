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
DEMO_SOURCE = codecaching.c

# Library settings
LIB_TARGET = codecaching.so
LIB_CFLAGS = $(CFLAGS) -fPIC -shared

# Default target
all: $(LIB_TARGET)


# Build shared library
$(LIB_TARGET): $(DEMO_SOURCE)
	$(CC) $(LIB_CFLAGS) -o $(LIB_TARGET) $(DEMO_SOURCE) $(LDFLAGS)

# Clean build artifacts
clean:
	rm -f $(LIB_TARGET) *.o *.so

# Show Python configuration (for debugging)
info:
	@echo "Python includes: $(PYTHON_INCLUDE)"
	@echo "Python libs: $(PYTHON_LIBS)"
	@echo "Python version: $(PYTHON_VERSION)"
	@echo "Compiler flags: $(CFLAGS)"
	@echo "Linker flags: $(LDFLAGS)"


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

