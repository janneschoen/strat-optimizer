
FILES = backtesting.c 0_SMA_crossover.c 1_RSI.c

OUTPUT = compute

CC = gcc
CFLAGS = -Wall -g

$(OUTPUT): $(FILES)
	$(CC) $(CFLAGS) -o $@ $^ -lm

clean:
	rm -f $(OUTPUT)
