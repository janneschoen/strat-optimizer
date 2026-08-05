
FILES := $(wildcard src/*.c src/strategies/*.c)
LIB_FILES := $(filter-out src/core.c, $(FILES))

OUTPUT = compute
LIBRARY = libengine.so

CC = gcc
CFLAGS = -Wall -O2 -march=native -fopenmp -Isrc

$(OUTPUT): $(FILES)
	$(CC) $(CFLAGS) -o $@ $^ -lm

$(LIBRARY): $(LIB_FILES)
	$(CC) $(CFLAGS) -shared -fPIC -o $@ $^ -lm

all: $(OUTPUT) $(LIBRARY)

clean:
	rm -f $(OUTPUT) $(LIBRARY)
