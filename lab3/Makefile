CC=gcc
CFLAGS=-std=gnu99 -Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined
LDLIBS=-lpthread -lm

all: zad1

zad1: zad1.c
	$(CC) $(CFLAGS) $(LDFLAGS) zad1.c -o zad1 $(LDLIBS)


clean:
	rm -f zad1