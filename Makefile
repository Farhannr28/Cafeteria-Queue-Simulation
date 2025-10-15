CC = gcc
CFLAGS = -I./src/SIMLIB
SRC = src/main.c src/SIMLIB/simlib.c
OUT = main.out

all: run

build: $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC) -lm

run: build
	./$(OUT)

clean:
	rm -f $(OUT)
