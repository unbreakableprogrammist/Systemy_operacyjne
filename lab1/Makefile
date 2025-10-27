CC=gcc
CFLAGS=-Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined
all : prog1 prog2
prog3: prog3.c
	${CC} ${CFLAGS} ${LDFLAGS} -o prog3 prog3.c

prog2: prog2.c
	${CC} ${CFLAGS} ${LDFLAGS} -o prog2 prog2.c

prog1: prog1.c
	${CC} ${CFLAGS} ${LDFLAGS} -o prog1 prog1.c

.PHONY: clean all
clean:
	rm -f prog1 prog2 prog3
