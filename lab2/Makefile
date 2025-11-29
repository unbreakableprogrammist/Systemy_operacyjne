CC=gcc
CFLAGS=-Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined
prog1: prog1.c
	$(CC) $(CFLAGS) -o prog1 prog1.c $(LDFLAGS)
prog2: prog2.c
	$(CC) $(CFLAGS) -o prog2 prog2.c $(LDFLAGS)
prog3: prog3.c
	$(CC) $(CFLAGS) -o prog3 prog3.c $(LDFLAGS)
prog4: prog4.c
	$(CC) $(CFLAGS) -o prog4 prog4.c $(LDFLAGS)
prog4b: prog4b.c
	$(CC) $(CFLAGS) -o prog4b prog4b.c $(LDFLAGS)
.PHONY: clean all
clean:
	rm -f prog1 prog2 prog3 prog4 prog6 prog5