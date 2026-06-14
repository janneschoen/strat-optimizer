
FILES := $(wildcard src/*.c src/strategies/*.c)

OUTPUT = compute

CC = gcc
CFLAGS = -Wall -g -fopenmp -Isrc

$(OUTPUT): $(FILES)
	$(CC) $(CFLAGS) -o $@ $^ -lm

clean:
	rm -f $(OUTPUT)
