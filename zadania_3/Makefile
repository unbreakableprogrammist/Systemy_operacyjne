CC=gcc
CFLAGS=-std=gnu99 -Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined
LDLIBS=-lpthread -lm

zad3: zadanie3.c
	$(CC) $(CFLAGS) $(LDFLAGS) zadanie3.c -o zadanie3 $(LDLIBS)
zad2: zadanie2.c
	$(CC) $(CFLAGS) $(LDFLAGS) zadanie2.c -o zadanie2 $(LDLIBS)


zad1: zadanie1.c
	$(CC) $(CFLAGS) $(LDFLAGS) zadanie1.c -o zadanie1 $(LDLIBS)

.PHONY: clean
clean:
	rm -f zadanie1 zadanie2 zadanie3 