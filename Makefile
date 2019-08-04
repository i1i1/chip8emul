CC=gcc
CFLAGS=-Wall -O3
LDFLAGS=-lSDL2 -lm
BIN=emul
BIN_DIS=dis
ROM=test/PONG2

all:
	$(CC) $(CFLAGS) emul.c -o $(BIN) $(LDFLAGS)

test: all
	./$(BIN) $(ROM)

debug: CFLAGS += -DDEBUG
debug: all test

dis_prep:
	$(CC) $(CFLAGS) disassemble.c -o $(BIN_DIS)

dis: dis_prep
	./$(BIN_DIS) $(ROM)

clean:
	-rm -f $(BIN_DIS)
	-rm -f $(BIN)



