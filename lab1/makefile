CFLAGS = -c -Wall -Wextra -Wpedantic -std=c11

all: main

main: main.o
	gcc main.o -o main

main.o: main.c
	gcc $(CFLAGS) main.c

clean:
	rm -rf *.o main

.PHONY: all clean
