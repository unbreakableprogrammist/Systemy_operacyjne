CC=gcc
CFLAGS=-Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined

prog1: zadanie1.c
	$(CC) $(CFLAGS) -o prog1 zadanie1.c $(LDFLAGS)

.PHONY: clean all
clean:
	rm -f prog1 prog2 prog3 prog4 prog6 prog5 *.txt
	