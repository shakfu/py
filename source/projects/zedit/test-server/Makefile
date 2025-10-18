# Makefile for standalone test server
# No Max/MSP dependencies - just Python + Mongoose

PYTHON_CONFIG ?= python3.13-config
PYTHON_CFLAGS := $(shell $(PYTHON_CONFIG) --includes)
PYTHON_LDFLAGS := $(shell $(PYTHON_CONFIG) --ldflags --embed 2>/dev/null || $(PYTHON_CONFIG) --ldflags)

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I../ $(PYTHON_CFLAGS)
LDFLAGS = $(PYTHON_LDFLAGS)

TARGET = test-server
SOURCES = test-server.c ../mongoose.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)
	@echo ""
	@echo "Test server built!"
	@echo "Run: ./$(TARGET)"

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
