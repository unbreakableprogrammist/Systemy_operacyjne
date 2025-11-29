CC=gcc
CFLAGS=-Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined
prog1: prog1.c
	$(CC) $(CFLAGS) -o prog1 prog1.c $(LDFLAGS)
prog2: prog2.c
	$(CC) $(CFLAGS) -o prog2 prog2.c $(LDFLAGS)

.PHONY: clean all
clean:
	rm -f prog1 prog2 prog3 prog4 prog6 prog5