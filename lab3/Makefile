CC=gcc
CFLAGS=-std=gnu99 -Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined
LDLIBS=-lpthread -lm

zad4: zad4.c
	$(CC) $(CFLAGS) $(LDFLAGS) zad4.c -o zad4 $(LDLIBS)
zad3: zad3.c
	$(CC) $(CFLAGS) $(LDFLAGS) zad3.c -o zad3 $(LDLIBS)
zad2: zad2.c
	$(CC) $(CFLAGS) $(LDFLAGS) zad2.c -o zad2 $(LDLIBS)
zad1: zad1.c
	$(CC) $(CFLAGS) $(LDFLAGS) zad1.c -o zad1 $(LDLIBS)
.PHONY: clean
clean:
	rm -f zad1 zad2 zad3 zad4