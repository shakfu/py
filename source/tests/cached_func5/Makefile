# Makefile for Integrated Smart Cache Demo

PYTHON := python3
PYTHON_INCLUDE := $(shell python3-config --includes)
PYTHON_LIBS := $(shell python3-config --ldflags)

CC := gcc
CFLAGS := -Wall -Wextra -O2 -std=c99 $(PYTHON_INCLUDE)
LDFLAGS := $(PYTHON_LIBS) -lpthread -lpython3.13

TARGET := pysmart_cache_demo
SOURCE := pysmart_cache_demo.c
HEADER := pysmart_cache.h

all: $(TARGET)

$(TARGET): $(SOURCE) $(HEADER)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean
