CC=gcc
CFLAGS=-g -std=gnu99 -Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined
LDLIBS=-lpthread -lm


.PHONY: all clean
all: zad1 zad2 zad3 zad4


zad3: zad3.c
	$(CC) $(CFLAGS) $(LDFLAGS) zad3.c -o zad3 $(LDLIBS)
zad2: zad2.c
	$(CC) $(CFLAGS) $(LDFLAGS) zad2.c -o zad2 $(LDLIBS)
zad1: zad1.c
	$(CC) $(CFLAGS) $(LDFLAGS) zad1.c -o zad1 $(LDLIBS)
clean:
	rm -f zad1 zad2 zad3 zad4