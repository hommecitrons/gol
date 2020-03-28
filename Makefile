CC = gcc
CFLAGS = -g -Wall
LFLAGS = -lm -lSDL2 

NAME = gol

SDIR = src
ODIR = obj
SRC = $(wildcard $(SDIR)/*.c)
OBJ = $(patsubst $(SDIR)/%.c,$(ODIR)/%.o,$(SRC))

default: $(NAME)

$(NAME): $(OBJ) 
	$(CC) -o $@ $^ $(LFLAGS)

$(ODIR)/%.o: $(SDIR)/%.c
	mkdir -p $(ODIR)
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
.PHONY: run

run: $(NAME)
	./$(NAME)

clean: 
	rm -rf $(ODIR)
	rm -f $(NAME)
