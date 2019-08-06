CC=gcc
CFLAGS=-Wall -Wextra
LDFLAGS=-lSDL2 -lm
OUT=emul
ROM=test/PONG2

all: emul.c
	$(CC) $(CFLAGS) emul.c -o $(OUT) $(LDFLAGS)

test: all
	./$(OUT) $(ROM)


clean:
	-rm -f $(OUT)



