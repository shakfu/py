INCLUDES := $(shell python3-config --include)
LDFLAGS  := $(shell python3-config --ldflags)

.PHONY: all build clean

all: build


build:
	@$(CC) -o main $(INCLUDES) $(LDFLAGS) -lpython3.13 cached_func.c


clean:
	@rm main