CC = gcc
CFLAGS = -I./src/SIMLIB
SRC = src/main-bacin.c src/SIMLIB/simlib.c
OUT = main.out

all: build

build: $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC) -lm

run-1:
	./$(OUT) config/1-base.in output/1-base.out

run-2:
	./$(OUT) config/2-a1.in output/2-a1.out

run-3:
	./$(OUT) config/3-a2.in output/3-a2.out

run-4:
	./$(OUT) config/4-a3.in output/4-a3.out

run-5:
	./$(OUT) config/5-b1.in output/5-b1.out

run-6:
	./$(OUT) config/6-b2.in output/6-b2.out

run-7:
	./$(OUT) config/7-b3.in output/7-b3.out

run-8:
	./$(OUT) config/8-seven-emp.in output/8-seven-emp.out

clean:
	rm -f $(OUT)
