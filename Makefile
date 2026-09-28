FILES := $(wildcard src/*.c src/strategies/*.c)
LIB_FILES := $(filter-out src/core.c, $(FILES))

OUTPUT = engine-cli
LIBRARY = libengine.so

CC = gcc
CFLAGS = -Wall -O2 -march=native -fopenmp -Isrc

.PHONY: all clean

all: $(OUTPUT) $(LIBRARY)

$(OUTPUT): $(FILES)
	$(CC) $(CFLAGS) -o $@ $^ -lm

$(LIBRARY): $(LIB_FILES)
	$(CC) $(CFLAGS) -shared -fPIC -o $@ $^ -lm

clean:
	rm -f $(OUTPUT) $(LIBRARY)
