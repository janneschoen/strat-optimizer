
FILES = backtesting.c 01-SMA-Crossover.c 02-RSI.c

OUTPUT = compute

CC = gcc
CFLAGS = -Wall -g

$(OUTPUT): $(FILES)
	$(CC) $(CFLAGS) -o $@ $^ -lm

clean:
	rm -f $(OUTPUT)
