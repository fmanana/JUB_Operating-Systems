CC = gcc
CFLAGS = -Wall
LINKS = -lpthread

all: main

main: coins.o
	$(CC) $(CFLAGS) -o coins coins.c $(LINKS)

coins.o: coins.c
	$(CC) $(CFLAGS) -c coins.c

clean:
	rm -f *.o