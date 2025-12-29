CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lncurses

SRC = main.c ui.c process.c
OBJ = $(SRC:.c=.o)
EXEC = process_manager

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(EXEC) $(LDFLAGS)

clean:
	rm -f $(OBJ) $(EXEC)

