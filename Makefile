
FILES := $(wildcard *.c)

OUTPUT = compute

CC = gcc
CFLAGS = -Wall -g

$(OUTPUT): $(FILES)
	$(CC) $(CFLAGS) -o $@ $^ -lm

clean:
	rm -f $(OUTPUT)
