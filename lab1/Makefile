CC=gcc
CFLAGS=-Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined
all : prog1
prog1: prog1.c
	${CC} ${CFLAGS} ${LDFLAGS} -o prog1 prog1.c

.PHONY: clean all
clean:
	prog1