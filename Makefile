FILES = main.c strategy.c backtest.c tuning.c regression.c graphics.c
TARGET = quant5k

IDIR = .

CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -I$(IDIR)
LDFLAGS = -lm

.PHONY: clean all
all: $(TARGET)

$(TARGET): $(FILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(FILES) $(LDFLAGS)

clean:
	rm $(TARGET)
