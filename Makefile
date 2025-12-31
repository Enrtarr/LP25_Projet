CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -g
LDFLAGS = -lncurses

SRC = main.c ui.c process.c network.c
OBJ = $(SRC:.c=.o)
BIN = process_manager

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all clean
