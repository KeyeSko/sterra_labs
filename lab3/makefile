CFLAGS = -c -Wall -Wextra -Wpedantic -std=c11

all: main

main: main.o hash_map.o
	gcc hash_map.o main.o -o main

main.o: main.c
	gcc $(CFLAGS) main.c

hash_map.o: hash_map.c
	gcc $(CFLAGS) hash_map.c

clean:
	rm -rf *.o main
