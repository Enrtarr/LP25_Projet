CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS = -lncurses

SRC = ui.c main.c
OBJ = $(SRC:.c=.o)
EXEC = simple_manager

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(OBJ) -o $(EXEC) $(LDFLAGS)

clean:
	rm -f $(OBJ) $(EXEC)