
CFLAGS:=-g -Wall -O2

BIN=enigma
SRC=src/main.c
OBJ=obj/main.c.o
OBJDIR=obj/

.PHONY: all run clean mrproper

all: $(BIN)

run: $(BIN)
	./$(BIN)

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c -o $@ $^

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -rf obj/*

mrproper: clean
	rm -rf $(BIN)
