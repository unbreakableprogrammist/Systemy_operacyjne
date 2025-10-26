CC=gcc
CFLAGS=-Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined

all: prog1 prog2 prog3 prog4.c prog5.c
prog8: prog8.c
	${CC} ${CFLAGS} ${LDFLAGS} -o prog8 prog8.c
prog7: prog7.c
	${CC} ${CFLAGS} ${LDFLAGS} -o prog7 prog7.c
prog6: prog6.c
	${CC} ${CFLAGS} ${LDFLAGS} -o prog6 prog6.c
prog5: prog5.c
	${CC} ${CFLAGS} ${LDFLAGS} -o prog5 prog5.c
prog4: prog4.c 
	${CC} ${CFLAGS} ${LDFLAGS} -o prog4 prog4.c
prog1: prog1.c hello.h
	gcc ${CFLAGS} ${LDFLAGS} -o prog1 prog1.c
prog2: prog2.c hello.h
	gcc ${CFLAGS} ${LDFLAGS} -o prog2 prog2.c
prog3: prog3.c
	gcc ${CFLAGS} ${LDFLAGS} -o prog3 prog3.c
.PHONY: clean all
clean:
	rm prog1 prog2 prog3 prog4 prog5 prog6 prog7 prog8
