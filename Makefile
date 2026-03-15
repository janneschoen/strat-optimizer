
FILES = backtesting.c strategy0.c

OUTPUT = compute

CC = gcc
CFLAGS = -Wall -g

$(OUTPUT): $(FILES)
	$(CC) $(CFLAGS) -o $@ $^ -lm

clean:
	rm -f $(OUTPUT)
