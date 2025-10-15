CC = gcc
CFLAGS = -I./src/SIMLIB
SRC = src/main-bacin.c src/SIMLIB/simlib.c
OUT = main.out

all: build

build: $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC) -lm

clean:
	rm -f $(OUT)
