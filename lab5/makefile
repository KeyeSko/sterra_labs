CFLAGS = -c -Wall -Wextra -Wpedantic -std=c11

all: main

main: main.o liblogger.a
	gcc main.o -o main -lpthread -L. -llogger -rdynamic

liblogger.a: logger.o
	ar rcs liblogger.a logger.o

main.o: main.c
	gcc $(CFLAGS) main.c

logger.o: logger.c logger.h
	gcc $(CFLAGS) logger.c

clean:
	rm -rf *.o main *.log
