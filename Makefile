SOURCE_DIR = .

FILES = $(shell find $(SOURCE_DIR) -name "*.c" | grep -v "regression.c")

OUTPUT = optimizer

CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -I. -lcjson -g

$(OUTPUT): $(FILES)
	$(CC) $(CFLAGS) -o $@ $^ -lm

clean:
	rm -f $(OUTPUT)
